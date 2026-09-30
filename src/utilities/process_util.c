#include "utilities/process_util.h"
#include "utilities/str_util.h"

#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int process_split_args(const char *command, char **argv, int max_args, char *storage, size_t storage_size) {
    if (!command || !storage || storage_size == 0) return -1;
    int argc = 0;
    size_t used = 0;
    const char *p = command;
    while (*p) {
        while (*p == ' ' || *p == '\t') p++;
        if (!*p) break;
        if (argc >= max_args - 1) return -1;
        argv[argc++] = storage + used;
        char quote = 0;
        while (*p && (quote || (*p != ' ' && *p != '\t'))) {
            if (!quote && (*p == '"' || *p == '\'')) { quote = *p++; continue; }
            if (quote && *p == quote) { quote = 0; p++; continue; }
            if (used + 1 >= storage_size) return -1;
            storage[used++] = *p++;
        }
        if (used + 1 >= storage_size) return -1;
        storage[used++] = '\0';
    }
    argv[argc] = NULL;
    return argc > 0 ? argc : -1;
}

static void redirect_stdio_to_null(void) {
    int fd = open("/dev/null", O_RDWR);
    if (fd < 0) return;
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > STDERR_FILENO) close(fd);
}

int process_spawn_detached(char *const argv[]) {
    if (!argv || !argv[0]) return -1;
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        setsid();
        pid_t grandchild = fork();
        if (grandchild != 0) _exit(grandchild < 0 ? 1 : 0);
        redirect_stdio_to_null();
        execvp(argv[0], argv);
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? 0 : -1;
}

int process_on_path(const char *name) {
    if (!name || !*name) return 0;
    if (strchr(name, '/')) return access(name, X_OK) == 0;
    const char *path = getenv("PATH");
    if (!path) return 0;
    char dirs[4096];
    str_copy(dirs, sizeof(dirs), path);
    char *save = NULL;
    for (char *dir = strtok_r(dirs, ":", &save); dir; dir = strtok_r(NULL, ":", &save)) {
        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", dir, name);
        if (access(full, X_OK) == 0) return 1;
    }
    return 0;
}
