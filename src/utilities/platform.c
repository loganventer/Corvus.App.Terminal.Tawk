#include "utilities/platform.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int platform_is_macos(void) {
#ifdef __APPLE__
    return 1;
#else
    return 0;
#endif
}

int platform_is_wsl(void) {
    static int cached = -1;
    if (cached >= 0) return cached;
    cached = 0;
    FILE *f = fopen("/proc/sys/kernel/osrelease", "r");
    if (f) {
        char buf[256] = {0};
        if (fgets(buf, sizeof(buf), f)) cached = strstr(buf, "microsoft") || strstr(buf, "WSL");
        fclose(f);
    }
    return cached;
}

int platform_wsl_interop(void) {
    return platform_is_wsl() && access("/proc/sys/fs/binfmt_misc/WSLInterop", F_OK) == 0;
}
