/* The control socket itself and the rules behind it: a real Unix socket
 * in a temporary folder (its mode, lines split and joined, over-long lines,
 * a socket left behind, a second listener), the shell command arguments,
 * and the automation policy, rate limiter and chat resolver. */
#include "clients/cli/control_options.h"
#include "core/settings.h"
#include "engines/automation_policy.h"
#include "engines/chat_reference_resolver.h"
#include "engines/rate_limiter.h"
#include "core/settings_schema.h"
#include "infrastructure/unix_control_client.h"
#include "infrastructure/unix_control_transport.h"
#include "utilities/str_util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static int failures = 0;

#define CHECK(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); failures++; } } while (0)

/* Polls until something arrives or a few tries pass. */
static int poll_some(IControlTransport *t, ControlInbound *out, int max) {
    for (int i = 0; i < 50; i++) {
        int n = t->poll(t, out, max);
        if (n > 0) return n;
        usleep(10000);
    }
    return 0;
}

static void test_socket(const char *dir) {
    char path[600];
    snprintf(path, sizeof(path), "%s/run/control.sock", dir);
    IControlTransport *t = unix_control_transport_create();
    char why[160] = "";
    CHECK(t->listen(t, path, why, sizeof(why)) == 0, "it listens, making its folder");
    struct stat st;
    CHECK(lstat(path, &st) == 0 && (st.st_mode & 0777) == 0600 && S_ISSOCK(st.st_mode), "the socket is 0600");
    char folder[600];
    snprintf(folder, sizeof(folder), "%s/run", dir);
    CHECK(stat(folder, &st) == 0 && (st.st_mode & 0777) == 0700, "its folder is 0700");

    IControlClient *c = unix_control_client_create();
    CHECK(c->connect(c, path) == 0, "a client of the same user connects");
    ControlInbound in[8];
    int n = poll_some(t, in, 8);
    CHECK(n >= 1 && in[0].kind == CONTROL_INBOUND_OPENED, "the connection is reported");
    int conn = n ? in[0].conn : 0;
    for (int i = 0; i < n; i++) control_inbound_dispose(&in[i]);

    int raw = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    str_copy(addr.sun_path, sizeof(addr.sun_path), path);
    CHECK(connect(raw, (struct sockaddr *)&addr, sizeof(addr)) == 0, "a second client connects too");
    ssize_t w = write(raw, "{\"a\":", 5);
    n = poll_some(t, in, 8);
    int lines = 0;
    for (int i = 0; i < n; i++) { lines += in[i].kind == CONTROL_INBOUND_LINE; control_inbound_dispose(&in[i]); }
    CHECK(w == 5 && lines == 0, "half a line is kept");
    w = write(raw, "1}\r\n{\"b\":2}\n", 13);
    int got = 0;
    char first[64] = "";
    for (int tries = 0; tries < 50 && got < 2; tries++) {
        n = t->poll(t, in, 8);
        for (int i = 0; i < n; i++) {
            if (in[i].kind == CONTROL_INBOUND_LINE && got++ == 0) str_copy(first, sizeof(first), in[i].line);
            control_inbound_dispose(&in[i]);
        }
        usleep(10000);
    }
    CHECK(got == 2 && !strcmp(first, "{\"a\":1}"), "lines are joined and split, without the carriage return");

    CHECK(t->send(t, conn, "{\"ok\":true}") == 0, "a line goes back");
    char *line = c->read_line(c, 1000);
    CHECK(line && !strcmp(line, "{\"ok\":true}"), "and the client reads it whole");
    free(line);

    fcntl(raw, F_SETFL, fcntl(raw, F_GETFL, 0) | O_NONBLOCK);      /* the server stops reading; never block on it */
    char *big = malloc(1100 * 1024);
    memset(big, 'x', 1100 * 1024);
    size_t sent = 0;
    int closed_early = 0;
    while (sent < 1100 * 1024) {
        ssize_t k = send(raw, big + sent, 1100 * 1024 - sent, MSG_NOSIGNAL);
        n = t->poll(t, in, 8);
        int gone = 0;
        for (int i = 0; i < n; i++) { gone |= in[i].kind == CONTROL_INBOUND_CLOSED; control_inbound_dispose(&in[i]); }
        if (gone) { sent = 1100 * 1024; closed_early = 1; break; }
        if (k > 0) sent += (size_t)k;
        else if (errno != EAGAIN && errno != EWOULDBLOCK) break;
        else usleep(1000);
    }
    free(big);
    int closed = closed_early;
    for (int tries = 0; tries < 50 && !closed; tries++) {
        n = t->poll(t, in, 8);
        for (int i = 0; i < n; i++) { closed |= in[i].kind == CONTROL_INBOUND_CLOSED; control_inbound_dispose(&in[i]); }
        usleep(10000);
    }
    CHECK(closed, "a line over 1 MiB closes that client");
    close(raw);

    IControlTransport *second = unix_control_transport_create();
    CHECK(second->listen(second, path, why, sizeof(why)) != 0 && strstr(why, "another"), "a second tawk cannot take the socket");
    second->destroy(second);
    c->destroy(c);
    t->destroy(t);
    CHECK(lstat(path, &st) != 0, "the socket is removed when tawk stops");

    int stale = socket(AF_UNIX, SOCK_STREAM, 0);
    CHECK(bind(stale, (struct sockaddr *)&addr, sizeof(addr)) == 0, "a socket is left behind");
    close(stale);
    t = unix_control_transport_create();
    CHECK(t->listen(t, path, why, sizeof(why)) == 0, "one left behind by a tawk that crashed is replaced");
    t->destroy(t);
}

static void test_options(void) {
    ControlOptions o;
    char *send[] = { "Mom", "on", "my", "way" };
    CHECK(control_options_parse(CONTROL_COMMAND_SEND, 4, send, &o) == 0 && o.chat_count == 1 && o.word_count == 3, "tawk send CHAT words");
    char *piped[] = { "Mom", "-" };
    CHECK(control_options_parse(CONTROL_COMMAND_SEND, 2, piped, &o) == 0 && o.from_stdin, "tawk send CHAT - reads standard input");
    char *empty[] = { "Mom" };
    CHECK(control_options_parse(CONTROL_COMMAND_SEND, 1, empty, &o) != 0, "a send without text is refused");
    char *tail[] = { "Mom", "Work", "--json" };
    CHECK(control_options_parse(CONTROL_COMMAND_TAIL, 3, tail, &o) == 0 && o.chat_count == 2 && o.json, "tawk tail chats --json");
    char *line[] = { "--format", "{unread}" };
    CHECK(control_options_parse(CONTROL_COMMAND_STATUS_LINE, 2, line, &o) == 0 && !strcmp(o.format, "{unread}"), "status-line --format");
    CHECK(control_command_kind_of("unread") == CONTROL_COMMAND_UNREAD && control_command_kind_of("--help") == CONTROL_COMMAND_NONE,
          "commands are told from options");
}

static Chat chat(const char *jid, const char *name) {
    Chat c;
    chat_init(&c, jid);
    str_copy(c.name, sizeof(c.name), name);
    c.is_locked = 0;
    return c;
}

static void test_rules(void) {
    Settings s;
    settings_set_defaults(&s);
    CHECK(automation_policy_write(&s, CONTROL_ORIGIN_CLI, WRITE_KIND_SEND) == AUTOMATION_VERDICT_REFUSE, "read access writes nothing");
    str_copy(s.automation_access, sizeof(s.automation_access), "send");
    CHECK(automation_policy_write(&s, CONTROL_ORIGIN_MCP, WRITE_KIND_SEND) == AUTOMATION_VERDICT_ASK, "a model's send asks you");
    CHECK(automation_policy_write(&s, CONTROL_ORIGIN_CLI, WRITE_KIND_SEND) == AUTOMATION_VERDICT_ALLOW, "your shell's does not");
    s.automation_confirm_cli = 1;
    CHECK(automation_policy_write(&s, CONTROL_ORIGIN_CLI, WRITE_KIND_SEND) == AUTOMATION_VERDICT_ASK, "unless you ask it to");
    CHECK(automation_policy_write(&s, CONTROL_ORIGIN_MCP, WRITE_KIND_MANAGE) == AUTOMATION_VERDICT_REFUSE, "send access manages nothing");
    str_copy(s.automation_access, sizeof(s.automation_access), "manage");
    s.automation_confirm_cli = 0;
    CHECK(automation_policy_write(&s, CONTROL_ORIGIN_CLI, WRITE_KIND_DESTRUCTIVE) == AUTOMATION_VERDICT_ASK, "destructive always asks");
    CHECK(automation_policy_risk("delete_chat", WRITE_KIND_DESTRUCTIVE) == APPROVAL_RISK_HIGH &&
          automation_policy_risk("react", WRITE_KIND_SEND) == APPROVAL_RISK_LOW &&
          automation_policy_risk("send_message", WRITE_KIND_SEND) == APPROVAL_RISK_MEDIUM, "risks by what it does");
    CHECK(!automation_policy_setting_changeable(settings_schema_find(SETTING_CATEGORY_AUTOMATION, "access")) &&
          !automation_policy_setting_changeable(settings_schema_find(SETTING_CATEGORY_MEDIA, "video_player")) &&
          !automation_policy_setting_changeable(settings_schema_find(SETTING_CATEGORY_ADVANCED, "backend")) &&
          automation_policy_setting_changeable(settings_schema_find(SETTING_CATEGORY_APPEARANCE, "theme")),
          "permissions, commands and the backend stay out of reach");

    Chat list[4] = { chat("27820000001@s.whatsapp.net", "Mom"), chat("27820000009@s.whatsapp.net", "Mommy"),
                     chat("120363000000000001@g.us", "Work (team)"), chat("27820000002@s.whatsapp.net", "Secret") };
    list[3].is_locked = 1;
    CHECK(automation_policy_chat_allowed(&s, &list[0]) && !automation_policy_chat_allowed(&s, &list[3]), "locked chats are never allowed");
    list[1].soft_locked = 1;
    CHECK(!automation_policy_chat_allowed(&s, &list[1]), "nor soft-locked ones");
    list[1].soft_locked = 0;
    str_copy(s.automation_chats, sizeof(s.automation_chats), "Work (team), +27 82 000 0001");
    CHECK(automation_policy_chat_allowed(&s, &list[0]) && automation_policy_chat_allowed(&s, &list[2]) && !automation_policy_chat_allowed(&s, &list[1]),
          "the chat list matches names and numbers");
    s.automation_chats[0] = '\0';

    int found = -1, cand[4], nc = 0;
    CHECK(chat_reference_resolve(list, 4, &s, "mom", &found, cand, 4, &nc) == CHAT_RESOLUTION_FOUND && found == 0, "an exact name wins");
    CHECK(chat_reference_resolve(list, 4, &s, "Mo", &found, cand, 4, &nc) == CHAT_RESOLUTION_AMBIGUOUS && nc == 2, "a start that fits two is ambiguous");
    CHECK(chat_reference_resolve(list, 4, &s, "team", &found, cand, 4, &nc) == CHAT_RESOLUTION_FOUND && found == 2, "the start of any word");
    CHECK(chat_reference_resolve(list, 4, &s, "+27 82 000 0009", &found, cand, 4, &nc) == CHAT_RESOLUTION_FOUND && found == 1, "a phone number");
    CHECK(chat_reference_resolve(list, 4, &s, "Secret", &found, cand, 4, &nc) == CHAT_RESOLUTION_NOT_FOUND, "a locked chat is not found");

    RateLimiter r;
    rate_limiter_init(&r);
    int retry = 0, ok = 0;
    for (int i = 0; i < 5; i++) ok += rate_limiter_take(&r, 5, 1000, &retry);
    CHECK(ok == 5 && !rate_limiter_take(&r, 5, 1000, &retry) && retry >= 11 && retry <= 13, "five a minute, then wait about 12 s");
    CHECK(rate_limiter_take(&r, 5, 1000 + 12000, &retry), "and one more after it");
}

int main(void) {
    char dir[] = "/tmp/tawk-socket-XXXXXX";
    if (!mkdtemp(dir)) return 1;
    test_socket(dir);
    test_options();
    test_rules();
    char cmd[700];
    snprintf(cmd, sizeof(cmd), "rm -rf '%s'", dir);
    if (system(cmd) != 0) fprintf(stderr, "could not remove %s\n", dir);
    if (failures) { fprintf(stderr, "%d failure(s)\n", failures); return 1; }
    printf("ok: the control socket is private, keeps lines whole, refuses a second tawk, and the rules decide who sees and does what\n");
    return 0;
}
