#ifndef APP_UTILITIES_INSTANCE_LOCK_H
#define APP_UTILITIES_INSTANCE_LOCK_H

/* One tawk at a time per data folder: an exclusive lock on
 * <data_dir>/tawk.lock, held for as long as the process runs (the system
 * drops it when the process ends, even after a crash). Commands that
 * rewrite the database or restore a backup take it too, so they never run
 * beside a running tawk. */
typedef struct InstanceLock {
    int fd;
} InstanceLock;

/* Returns 0 when this process now holds the lock, -1 when another does
 * (or the file cannot be opened). */
int  instance_lock_acquire(InstanceLock *lock, const char *data_dir);
void instance_lock_release(InstanceLock *lock);

#endif
