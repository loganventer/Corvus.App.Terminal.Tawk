#ifndef APP_UTILITIES_PROCESS_CAPTURE_H
#define APP_UTILITIES_PROCESS_CAPTURE_H

#include <stddef.h>

/* Runs argv (no shell) and collects what it prints, up to size - 1 bytes,
 * waiting at most timeout_ms. Returns 0 when it started. */
int process_capture(char *const argv[], char *buf, size_t size, int timeout_ms);
/* Runs argv with stdout into out_fd, waiting at most timeout_ms. Returns 0
 * when it finished in time. */
int process_run_to_fd(char *const argv[], int out_fd, int timeout_ms);

#endif
