#ifndef APP_UTILITIES_PLATFORM_H
#define APP_UTILITIES_PLATFORM_H

int platform_is_macos(void);
/* Running inside Windows Subsystem for Linux. */
int platform_is_wsl(void);
/* WSL with Windows interop enabled (can launch .exe files). */
int platform_wsl_interop(void);

#endif
