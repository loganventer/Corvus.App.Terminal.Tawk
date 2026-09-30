#ifndef APP_UTILITIES_COLOR_UTIL_H
#define APP_UTILITIES_COLOR_UTIL_H

/* Nearest xterm-256 index for an RGB colour. */
short color_rgb_to_xterm256(int r, int g, int b);
/* Parses "#RRGGBB", "#RGB", "default", or a decimal 0-255 index. Returns -1
 * for "default" and -2 when the text is invalid. */
short color_parse(const char *text);
/* Nearest of the 8 basic curses colours for an xterm-256 index. */
short color_xterm256_to_8(short index);

#endif
