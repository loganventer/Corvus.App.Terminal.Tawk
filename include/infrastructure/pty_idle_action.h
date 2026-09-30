#ifndef APP_INFRASTRUCTURE_PTY_IDLE_ACTION_H
#define APP_INFRASTRUCTURE_PTY_IDLE_ACTION_H

#include "contracts/i_idle_action.h"

/* Runs the screensaver command in a pseudo-terminal that covers the whole
 * screen. The command never sees the keyboard: any key or mouse input stops
 * it. Window resizes are forwarded. The caller must have left curses mode. */
IIdleAction *pty_idle_action_create(void);

#endif
