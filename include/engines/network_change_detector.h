#ifndef APP_ENGINES_NETWORK_CHANGE_DETECTOR_H
#define APP_ENGINES_NETWORK_CHANGE_DETECTOR_H

#include <stdint.h>

/* Decides when the machine's network has changed, from a stream of
 * fingerprints. A new fingerprint counts only after it has stayed the same
 * for `settle_ms`, so an adapter that drops and comes back as it switches
 * (Wi-Fi to Ethernet, a hotspot joining) reports one change, not several. */
typedef struct NetworkChangeDetector {
    int64_t  settle_ms;
    int      primed;           /* the first fingerprint is the baseline, not a change */
    uint64_t current;
    uint64_t candidate;
    int64_t  candidate_since_ms;
} NetworkChangeDetector;

void network_change_detector_init(NetworkChangeDetector *detector, int64_t settle_ms);
/* Records one sample. True once, when a different fingerprint has settled. */
int  network_change_detector_observe(NetworkChangeDetector *detector, uint64_t fingerprint, int64_t now_ms);

#endif
