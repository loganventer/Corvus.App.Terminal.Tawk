#ifndef APP_CONTRACTS_I_NETWORK_MONITOR_H
#define APP_CONTRACTS_I_NETWORK_MONITOR_H

#include <stdint.h>

/* Watches the machine's network adapters. A connection opened on one
 * adapter usually hangs, without any error, once traffic moves to another
 * (Wi-Fi to Ethernet, or to a phone hotspot), so the messaging manager
 * reconnects when this reports a change. */
typedef struct INetworkMonitor {
    void *ctx;
    /* Polled every tick. True once per settled change of addresses. */
    int  (*changed)(struct INetworkMonitor *self, int64_t now_ms);
    void (*destroy)(struct INetworkMonitor *self);
} INetworkMonitor;

#endif
