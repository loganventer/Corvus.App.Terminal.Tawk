#ifndef APP_INFRASTRUCTURE_TERMINAL_GRAPHICS_H
#define APP_INFRASTRUCTURE_TERMINAL_GRAPHICS_H

/* True when the terminal is known to draw Sixel images: Windows Terminal,
 * iTerm2 3.5+, WezTerm, foot, mlterm, contour and xterm started with sixel support.
 * Inside tmux or screen it is false (they pass images on unreliably). */
int  terminal_graphics_sixel(void);
/* Pixel size of one character cell: from the terminal when it reports it,
 * else 10 x 20 (the cell size Windows Terminal scales Sixel images to). */
void terminal_graphics_cell_pixels(int *width, int *height);

#endif
