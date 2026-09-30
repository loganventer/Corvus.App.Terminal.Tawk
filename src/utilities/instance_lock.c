#include "utilities/instance_lock.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>

int instance_lock_acquire(InstanceLock *lock, const char *data_dir) {
    char path[1100];
    lock->fd = -1;
    snprintf(path, sizeof(path), "%s/tawk.lock", data_dir);
    int fd = open(path, O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
    if (fd < 0) return -1;
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) { close(fd); return -1; }
    lock->fd = fd;
    return 0;
}

void instance_lock_release(InstanceLock *lock) {
    if (lock->fd >= 0) { flock(lock->fd, LOCK_UN); close(lock->fd); }
    lock->fd = -1;
}
