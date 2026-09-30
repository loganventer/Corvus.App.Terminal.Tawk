#include "utilities/child_process.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

int child_process_start_io(ChildProcess *child, char *const argv[], int in_fd, int out_fd) {
    child->pid = -1;
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        setpgid(0, 0);
        int null_fd = open("/dev/null", O_RDWR);
        dup2(in_fd >= 0 ? in_fd : null_fd, STDIN_FILENO);
        dup2(out_fd >= 0 ? out_fd : null_fd, STDOUT_FILENO);
        dup2(null_fd, STDERR_FILENO);
        for (int fd = STDERR_FILENO + 1; fd < 256; fd++) close(fd);
        execvp(argv[0], argv);
        _exit(127);
    }
    child->pid = pid;
    return 0;
}

int child_process_start(ChildProcess *child, char *const argv[]) {
    return child_process_start_io(child, argv, -1, -1);
}

int child_process_running(ChildProcess *child) {
    if (child->pid <= 0) return 0;
    if (waitpid(child->pid, NULL, WNOHANG) == 0) return 1;
    child->pid = -1;
    return 0;
}

void child_process_stop(ChildProcess *child, int sig, int timeout_ms) {
    if (child->pid <= 0) return;
    kill(-child->pid, sig);
    for (int waited = 0; waited < timeout_ms; waited += 20) {
        if (waitpid(child->pid, NULL, WNOHANG) == child->pid) { child->pid = -1; return; }
        struct timespec ts = { 0, 20 * 1000000L };
        nanosleep(&ts, NULL);
    }
    kill(-child->pid, SIGKILL);
    waitpid(child->pid, NULL, 0);
    child->pid = -1;
}
