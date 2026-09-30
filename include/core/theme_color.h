#ifndef APP_CORE_THEME_COLOR_H
#define APP_CORE_THEME_COLOR_H

/* Foreground and background as xterm-256 indexes; -1 means the terminal
 * default. The painter derives 8-colour fallbacks when needed. */
typedef struct ThemeColor {
    short fg;
    short bg;
} ThemeColor;

#endif
