#include "utilities/file_settled.h"

#include <sys/stat.h>
#include <time.h>

#ifdef __APPLE__
#define st_mtim st_mtimespec
#endif

int file_settled(const char *path, int quiet_ms) {
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size <= 0) return 0;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    long long age_ms = (long long)(now.tv_sec - st.st_mtim.tv_sec) * 1000 + (now.tv_nsec - st.st_mtim.tv_nsec) / 1000000;
    return age_ms >= quiet_ms;
}
