#include "utilities/process_run.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int process_run_foreground(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execvp(argv[0], argv);
        _exit(127);
    }
    int status = 0;
    while (waitpid(pid, &status, 0) < 0) {}
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
