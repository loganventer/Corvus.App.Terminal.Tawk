#include "engines/network_change_detector.h"

void network_change_detector_init(NetworkChangeDetector *d, int64_t settle_ms) {
    d->settle_ms = settle_ms > 0 ? settle_ms : 0;
    d->primed = 0;
    d->current = d->candidate = 0;
    d->candidate_since_ms = 0;
}

int network_change_detector_observe(NetworkChangeDetector *d, uint64_t fingerprint, int64_t now_ms) {
    if (!d->primed) {
        d->primed = 1;
        d->current = d->candidate = fingerprint;
        return 0;
    }
    if (fingerprint != d->candidate) {
        d->candidate = fingerprint;
        d->candidate_since_ms = now_ms;
    }
    if (d->candidate == d->current || now_ms - d->candidate_since_ms < d->settle_ms) return 0;
    d->current = d->candidate;
    return 1;
}
