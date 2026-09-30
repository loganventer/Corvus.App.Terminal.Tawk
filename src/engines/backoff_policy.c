#include "engines/backoff_policy.h"

#include <stdlib.h>
#include <time.h>

static unsigned int jitter_seed(void) {
    static unsigned int seed = 0;
    if (seed == 0) seed = (unsigned int)time(NULL) ^ 0x5bd1e995u;
    return seed;
}

int64_t backoff_policy_delay_ms(const BackoffPolicy *policy, int attempt) {
    int64_t cap = policy->initial_ms > 0 ? policy->initial_ms : 1000;
    for (int i = 0; i < attempt && cap < policy->max_ms; i++) cap *= 2;
    if (cap > policy->max_ms) cap = policy->max_ms;
    unsigned int seed = jitter_seed() + (unsigned int)attempt * 2654435761u;
    int64_t half = cap / 2;
    return half + (int64_t)(rand_r(&seed) % (int)(half + 1));
}
