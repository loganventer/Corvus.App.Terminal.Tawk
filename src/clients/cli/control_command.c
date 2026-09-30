/* tawk send, tail, unread and status-line: small clients of a running
 * tawk's control socket, for scripts and status bars. */
#include "clients/cli/control_command.h"
#include "cJSON.h"
#include "utilities/app_info.h"
#include "utilities/clock_util.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ANSWER_WAIT_MS (6 * 60 * 1000)     /* longer than tawk waits for your approval */

static int request(IControlClient *c, const char *id, const char *op, cJSON *args) {
    cJSON *o = cJSON_CreateObject();
    cJSON_AddStringToObject(o, "id", id);
    cJSON_AddStringToObject(o, "op", op);
    if (args) cJSON_AddItemToObject(o, "args", args);
    char *line = cJSON_PrintUnformatted(o);
    cJSON_Delete(o);
    int rc = line ? c->send(c, line) : -1;
    free(line);
    return rc;
}

/* Waits for the answer to `id`, passing any events to `on_event`. Returns the parsed answer (caller deletes). */
static cJSON *answer(IControlClient *c, const char *id, int timeout_ms, void (*on_event)(const cJSON *evt)) {
    for (;;) {
        char *line = c->read_line(c, timeout_ms);
        if (!line) return NULL;
        cJSON *o = cJSON_Parse(line);
        free(line);
        if (!o) continue;
        const cJSON *got = cJSON_GetObjectItemCaseSensitive(o, "id");
        if (cJSON_GetObjectItemCaseSensitive(o, "evt")) {
            if (on_event) on_event(o);
            cJSON_Delete(o);
            continue;
        }
        if (cJSON_IsString(got) && !strcmp(got->valuestring, id)) return o;
        cJSON_Delete(o);
    }
}

static int failed(const cJSON *reply) {
    const cJSON *err = cJSON_GetObjectItemCaseSensitive(reply, "error");
    const cJSON *code = cJSON_GetObjectItemCaseSensitive(err, "code");
    const cJSON *msg = cJSON_GetObjectItemCaseSensitive(err, "message");
    fprintf(stderr, "tawk: %s\n", cJSON_IsString(msg) ? msg->valuestring : "it failed");
    const char *c = cJSON_IsString(code) ? code->valuestring : "";
    return !strcmp(c, "declined") || !strcmp(c, "not_allowed") || !strcmp(c, "timed_out") ? CONTROL_EXIT_REFUSED : CONTROL_EXIT_FAILED;
}

static const cJSON *result_of(const cJSON *reply) {
    return cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(reply, "ok")) ? cJSON_GetObjectItemCaseSensitive(reply, "result") : NULL;
}

static int hello(IControlClient *c) {
    cJSON *args = cJSON_CreateObject();
    cJSON_AddStringToObject(args, "client", "tawk");
    cJSON_AddStringToObject(args, "version", APP_VERSION);
    cJSON_AddNumberToObject(args, "protocol", 1);
    cJSON_AddStringToObject(args, "origin", "cli");
    if (request(c, "hello", "hello", args) != 0) return CONTROL_EXIT_NOT_RUNNING;
    cJSON *reply = answer(c, "hello", 5000, NULL);
    int rc = !reply ? CONTROL_EXIT_NOT_RUNNING : result_of(reply) ? CONTROL_EXIT_OK : failed(reply);
    cJSON_Delete(reply);
    return rc;
}

static char *read_stdin(void) {
    size_t cap = 4096, len = 0;
    char *buf = malloc(cap);
    size_t n;
    while (buf && (n = fread(buf + len, 1, cap - len - 1, stdin)) > 0) {
        len += n;
        if (len + 1 == cap) {
            char *bigger = realloc(buf, cap * 2);
            if (!bigger) break;
            buf = bigger;
            cap *= 2;
        }
    }
    if (buf) buf[len] = '\0';
    while (buf && len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';
    return buf;
}

static void waiting_note(const cJSON *evt) {
    const cJSON *name = cJSON_GetObjectItemCaseSensitive(evt, "evt");
    if (cJSON_IsString(name) && !strcmp(name->valuestring, "approval")) fprintf(stderr, "tawk: waiting for your answer in tawk (F3)\n");
}

static int run_send(IControlClient *c, const ControlOptions *o) {
    char *text = NULL;
    if (o->from_stdin) {
        text = read_stdin();
    } else {
        size_t n = 1;
        for (int i = 0; i < o->word_count; i++) n += strlen(o->words[i]) + 1;
        text = calloc(n, 1);
        for (int i = 0; text && i < o->word_count; i++) {
            if (i) strcat(text, " ");
            strcat(text, o->words[i]);
        }
    }
    if (!text || !*text) { free(text); fprintf(stderr, "tawk: nothing to send\n"); return CONTROL_EXIT_FAILED; }
    cJSON *args = cJSON_CreateObject();
    cJSON_AddStringToObject(args, "chat", o->chats[0]);
    cJSON_AddStringToObject(args, "text", text);
    free(text);
    if (request(c, "send", "send_message", args) != 0) return CONTROL_EXIT_NOT_RUNNING;
    cJSON *reply = answer(c, "send", ANSWER_WAIT_MS, waiting_note);
    if (!reply) { fprintf(stderr, "tawk: tawk went away before answering\n"); return CONTROL_EXIT_FAILED; }
    int rc = result_of(reply) ? CONTROL_EXIT_OK : failed(reply);
    cJSON_Delete(reply);
    return rc;
}

static const char *text_of(const cJSON *o, const char *name) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, name);
    return cJSON_IsString(v) ? v->valuestring : "";
}

static void print_message(const cJSON *evt, int json) {
    if (json) {
        char *line = cJSON_PrintUnformatted(evt);
        if (line) printf("%s\n", line);
        free(line);
        return;
    }
    const cJSON *chat = cJSON_GetObjectItemCaseSensitive(evt, "chat");
    const cJSON *msg = cJSON_GetObjectItemCaseSensitive(evt, "message");
    const cJSON *ts = cJSON_GetObjectItemCaseSensitive(msg, "ts");
    char when[32] = "";
    if (cJSON_IsNumber(ts)) clock_format_time((int64_t)ts->valuedouble, 1, when, sizeof(when));
    const char *text = text_of(msg, "text");
    printf("[%s] %s \xC2\xB7 %s: %s\n", when, text_of(chat, "name"), text_of(msg, "sender_name"), *text ? text : text_of(msg, "type"));
}

static int run_tail(IControlClient *c, const ControlOptions *o) {
    cJSON *args = cJSON_CreateObject();
    if (o->chat_count == 0) {
        cJSON_AddStringToObject(args, "chats", "all");
    } else {
        cJSON *list = cJSON_AddArrayToObject(args, "chats");
        for (int i = 0; i < o->chat_count; i++) cJSON_AddItemToArray(list, cJSON_CreateString(o->chats[i]));
    }
    if (request(c, "sub", "subscribe", args) != 0) return CONTROL_EXIT_NOT_RUNNING;
    cJSON *reply = answer(c, "sub", 5000, NULL);
    if (!reply || !result_of(reply)) { int rc = reply ? failed(reply) : CONTROL_EXIT_FAILED; cJSON_Delete(reply); return rc; }
    cJSON_Delete(reply);
    setvbuf(stdout, NULL, _IOLBF, 0);
    for (;;) {
        char *line = c->read_line(c, -1);
        if (!line) return CONTROL_EXIT_OK;                    /* tawk quit */
        cJSON *evt = cJSON_Parse(line);
        free(line);
        const char *name = text_of(evt, "evt");
        if (!strcmp(name, "message")) print_message(evt, o->json);
        int bye = !strcmp(name, "bye");
        cJSON_Delete(evt);
        if (bye) return CONTROL_EXIT_OK;
    }
}

static cJSON *unread_summary(IControlClient *c) {
    if (request(c, "unread", "unread_summary", NULL) != 0) return NULL;
    return answer(c, "unread", 5000, NULL);
}

static int run_unread(IControlClient *c, const ControlOptions *o) {
    cJSON *reply = unread_summary(c);
    const cJSON *r = result_of(reply);
    if (!r) { int rc = reply ? failed(reply) : CONTROL_EXIT_FAILED; cJSON_Delete(reply); return rc; }
    if (o->json) {
        char *line = cJSON_PrintUnformatted(r);
        if (line) printf("%s\n", line);
        free(line);
    } else {
        const cJSON *chat;
        cJSON_ArrayForEach(chat, cJSON_GetObjectItemCaseSensitive(r, "chats")) {
            const cJSON *n = cJSON_GetObjectItemCaseSensitive(chat, "unread");
            printf("%4d  %s%s\n", cJSON_IsNumber(n) ? n->valueint : 0, text_of(chat, "name"),
                   cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(chat, "unread_mention")) ? "  @" : "");
        }
    }
    cJSON_Delete(reply);
    return CONTROL_EXIT_OK;
}

/* "💬 {unread}" by default, with " @{mentions}" when someone mentioned you;
 * nothing at all when there is nothing unread or tawk is not running. */
static int run_status_line(IControlClient *c, const ControlOptions *o) {
    cJSON *reply = unread_summary(c);
    const cJSON *r = result_of(reply);
    int total = r ? cJSON_GetObjectItemCaseSensitive(r, "total")->valueint : 0;
    int mentions = r ? cJSON_GetObjectItemCaseSensitive(r, "mentions")->valueint : 0;
    int chats = r ? cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(r, "chats")) : 0;
    cJSON_Delete(reply);
    if (total == 0) { printf("\n"); return CONTROL_EXIT_OK; }
    const char *format = o->format ? o->format : mentions ? "\xF0\x9F\x92\xAC {unread} @{mentions}" : "\xF0\x9F\x92\xAC {unread}";
    for (const char *p = format; *p; p++) {
        if (!strncmp(p, "{unread}", 8)) { printf("%d", total); p += 7; }
        else if (!strncmp(p, "{mentions}", 10)) { printf("%d", mentions); p += 9; }
        else if (!strncmp(p, "{chats}", 7)) { printf("%d", chats); p += 6; }
        else putchar(*p);
    }
    printf("\n");
    return CONTROL_EXIT_OK;
}

int control_command_run(const ControlOptions *o, IControlClient *c, const char *socket_path) {
    if (c->connect(c, socket_path) != 0) {
        if (o->kind == CONTROL_COMMAND_STATUS_LINE) { printf("\n"); return CONTROL_EXIT_OK; }
        fprintf(stderr, "tawk: no tawk is listening on %s (is it running, with Settings > Automation > Agent access on?)\n", socket_path);
        return CONTROL_EXIT_NOT_RUNNING;
    }
    int rc = hello(c);
    if (rc != CONTROL_EXIT_OK) {
        if (o->kind == CONTROL_COMMAND_STATUS_LINE) { printf("\n"); return CONTROL_EXIT_OK; }
        return rc;
    }
    switch (o->kind) {
        case CONTROL_COMMAND_SEND:        return run_send(c, o);
        case CONTROL_COMMAND_TAIL:        return run_tail(c, o);
        case CONTROL_COMMAND_UNREAD:      return run_unread(c, o);
        case CONTROL_COMMAND_STATUS_LINE: return run_status_line(c, o);
        default:                          return CONTROL_EXIT_FAILED;
    }
}
