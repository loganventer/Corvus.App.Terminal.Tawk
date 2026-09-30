#include "infrastructure/ifaddrs_network_monitor.h"
#include "engines/network_change_detector.h"
#include "engines/network_fingerprint.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define SAMPLE_EVERY_MS 3000
#define SETTLE_MS       2000
#define MAX_ADDRESSES   64

typedef struct IfaddrsMonitor {
    NetworkChangeDetector detector;
    int64_t               next_sample_ms;
} IfaddrsMonitor;

/* IPv6 link-local addresses exist on every adapter and say nothing about
 * where traffic goes, so they are left out. */
static int is_link_local6(const struct sockaddr_in6 *a) {
    return a->sin6_addr.s6_addr[0] == 0xFE && (a->sin6_addr.s6_addr[1] & 0xC0) == 0x80;
}

static uint64_t sample(void) {
    struct ifaddrs *list = NULL;
    if (getifaddrs(&list) != 0) return 0;
    static char text[MAX_ADDRESSES][IFNAMSIZ + INET6_ADDRSTRLEN + 2];
    const char *entries[MAX_ADDRESSES];
    int count = 0;
    for (struct ifaddrs *it = list; it && count < MAX_ADDRESSES; it = it->ifa_next) {
        if (!it->ifa_addr || !(it->ifa_flags & IFF_UP) || !(it->ifa_flags & IFF_RUNNING)) continue;
        if (it->ifa_flags & IFF_LOOPBACK) continue;
        char address[INET6_ADDRSTRLEN] = "";
        if (it->ifa_addr->sa_family == AF_INET) {
            inet_ntop(AF_INET, &((struct sockaddr_in *)(void *)it->ifa_addr)->sin_addr, address, sizeof(address));
        } else if (it->ifa_addr->sa_family == AF_INET6) {
            const struct sockaddr_in6 *a6 = (const struct sockaddr_in6 *)(void *)it->ifa_addr;
            if (is_link_local6(a6)) continue;
            inet_ntop(AF_INET6, &a6->sin6_addr, address, sizeof(address));
        } else {
            continue;
        }
        snprintf(text[count], sizeof(text[count]), "%s %s", it->ifa_name, address);
        entries[count] = text[count];
        count++;
    }
    freeifaddrs(list);
    return network_fingerprint_of(entries, count);
}

static int monitor_changed(INetworkMonitor *self, int64_t now_ms) {
    IfaddrsMonitor *m = self->ctx;
    if (now_ms < m->next_sample_ms) return 0;
    m->next_sample_ms = now_ms + SAMPLE_EVERY_MS;
    return network_change_detector_observe(&m->detector, sample(), now_ms);
}

static void monitor_destroy(INetworkMonitor *self) {
    if (!self) return;
    free(self->ctx);
    free(self);
}

INetworkMonitor *ifaddrs_network_monitor_create(void) {
    INetworkMonitor *monitor = calloc(1, sizeof(*monitor));
    IfaddrsMonitor *m = calloc(1, sizeof(*m));
    if (!monitor || !m) { free(monitor); free(m); return NULL; }
    network_change_detector_init(&m->detector, SETTLE_MS);
    monitor->ctx = m;
    monitor->changed = monitor_changed;
    monitor->destroy = monitor_destroy;
    return monitor;
}
