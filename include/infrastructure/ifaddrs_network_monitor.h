#ifndef APP_INFRASTRUCTURE_IFADDRS_NETWORK_MONITOR_H
#define APP_INFRASTRUCTURE_IFADDRS_NETWORK_MONITOR_H

#include "contracts/i_network_monitor.h"

/* Samples the interfaces that are up (getifaddrs) every few seconds and
 * reports when their addresses change. Works on Linux, macOS and WSL in
 * mirrored networking mode, where Windows adapters appear inside Linux. */
INetworkMonitor *ifaddrs_network_monitor_create(void);

#endif
