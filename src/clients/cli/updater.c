#include "clients/cli/updater.h"
#include "utilities/app_info.h"
#include "utilities/path_util.h"
#include "utilities/process_capture.h"
#include "utilities/process_run.h"
#include "utilities/process_util.h"
#include "utilities/str_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define REPO_URL    APP_HOMEPAGE ".git"
#define INSTALL_URL "https://raw.githubusercontent.com/loganventer/tawk/main/install.sh"

/* /usr/local/bin/tawk -> /usr/local */
static int prefix_of(const char *self, char *out, size_t size) {
    char path[1024];
    str_copy(path, sizeof(path), self);
    char *slash = strrchr(path, '/');
    if (!slash) return -1;
    *slash = '\0';                                     /* .../bin */
    slash = strrchr(path, '/');
    if (!slash || strcmp(slash, "/bin") != 0) return -1;
    *slash = '\0';
    str_copy(out, size, path[0] ? path : "/");
    return 0;
}

static int confirm(const char *question) {
    printf("%s [Y/n] ", question);
    fflush(stdout);
    char reply[16] = "";
    if (!fgets(reply, sizeof(reply), stdin)) return 0;
    return reply[0] != 'n' && reply[0] != 'N';
}

int updater_run(const UpdaterOptions *o) {
    const char *mine = APP_COMMIT;
    printf("%s %s%s%.7s%s, installed at %s\n", APP_NAME, APP_VERSION, mine[0] ? " (" : "", mine, mine[0] ? ")" : "", o->self_path);
    fflush(stdout);
    char prefix[1024];
    if (prefix_of(o->self_path, prefix, sizeof(prefix)) != 0) {
        fprintf(stderr, "%s is not in an installed bin folder; update a source checkout with git pull and make instead\n", o->self_path);
        return 2;
    }
    if (!process_on_path("curl") || !process_on_path("bash")) {
        fprintf(stderr, "--update needs curl and bash\n");
        return 2;
    }

    /* The newest commit, to compare with this build and to show what gets installed. */
    char latest[64] = "";
    if (process_on_path("git")) {
        char out[512];
        char *ls[] = { "git", "ls-remote", REPO_URL, "refs/heads/main", NULL };
        if (process_capture(ls, out, sizeof(out), 15000) == 0 && strlen(out) >= 40) {
            memcpy(latest, out, 40);
            latest[40] = '\0';
            printf("Latest on GitHub: %.7s (main)\n", latest);
        }
    }
    if (!latest[0]) printf("Could not reach GitHub to compare versions.\n");
    int current = latest[0] && mine[0] && strcmp(latest, mine) == 0;
    if (current && !o->reinstall) {
        printf("%s is up to date. Use %s --reinstall to install it again anyway.\n", APP_NAME, APP_NAME);
        return 0;
    }
    int sidecar = access(o->sidecar_dir, F_OK) == 0 && strncmp(o->sidecar_dir, prefix, strlen(prefix)) == 0;
    printf("Will %s to %s with the %s backend%s\n", current ? "reinstall" : "install the latest", prefix,
           o->whatsmeow_built ? "whatsmeow" : "Node.js", sidecar && o->whatsmeow_built ? " and the Node.js backend" : "");
    if (!o->assume_yes && !confirm(current ? "Reinstall now?" : "Update now?")) { printf("Nothing changed.\n"); return 1; }
    fflush(stdout);                                    /* before the installer writes its own output */

    /* Download to a private file first (no pipe into a shell), then run it. */
    char tmpl[] = "/tmp/tawk-install.XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) { perror("mkstemp"); return 1; }
    close(fd);
    char *fetch[] = { "curl", "-fsSL", "--proto", "=https", "--tlsv1.2", "-o", tmpl, INSTALL_URL, NULL };
    if (process_run_foreground(fetch) != 0) {
        fprintf(stderr, "Could not download %s\n", INSTALL_URL);
        unlink(tmpl);
        return 1;
    }
    char *install[10];
    int n = 0;
    install[n++] = "bash";
    install[n++] = tmpl;
    install[n++] = "--prefix";
    install[n++] = prefix;
    install[n++] = "-y";
    if (!o->whatsmeow_built) install[n++] = "--no-whatsmeow";
    else if (sidecar) install[n++] = "--with-sidecar";
    install[n] = NULL;
    int rc = process_run_foreground(install);
    unlink(tmpl);
    if (rc == 0) printf("\nUpdated. Start %s again to use the new version.\n", APP_NAME);
    else fprintf(stderr, "\nThe installer stopped with status %d; your previous version is unchanged unless it said otherwise.\n", rc);
    return rc == 0 ? 0 : 1;
}
