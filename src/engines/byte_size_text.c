#include "engines/byte_size_text.h"

#include <stdio.h>

void byte_size_text(long long bytes, char *out, size_t size) {
    if (bytes < 0) bytes = 0;
    if (bytes < 1024) { snprintf(out, size, "%lld B", bytes); return; }
    static const char *const UNITS[] = { "KB", "MB", "GB" };
    double value = (double)bytes / 1024.0;
    int unit = 0;
    while (value >= 1024.0 && unit < 2) { value /= 1024.0; unit++; }
    if (value >= 100.0) snprintf(out, size, "%.0f %s", value, UNITS[unit]);
    else if (value >= 10.0 && unit == 0) snprintf(out, size, "%.0f %s", value, UNITS[unit]);
    else snprintf(out, size, "%.1f %s", value, UNITS[unit]);
}
