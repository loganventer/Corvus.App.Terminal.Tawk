#include "engines/receipt_elapsed_text.h"

#include <stdio.h>

void receipt_elapsed_text(int64_t sent_at, int64_t at, char *out, size_t size) {
    if (size == 0) return;
    out[0] = '\0';
    if (sent_at <= 0 || at <= sent_at) return;
    int64_t s = at - sent_at;
    if (s < 60) {
        snprintf(out, size, "%lld s after it was sent", (long long)s);
    } else if (s < 3600) {
        snprintf(out, size, "%lld min %lld s after it was sent", (long long)(s / 60), (long long)(s % 60));
    } else if (s < 86400) {
        snprintf(out, size, "%lld h %lld min after it was sent", (long long)(s / 3600), (long long)((s % 3600) / 60));
    } else {
        snprintf(out, size, "%lld d %lld h after it was sent", (long long)(s / 86400), (long long)((s % 86400) / 3600));
    }
}
