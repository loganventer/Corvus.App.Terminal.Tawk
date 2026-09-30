#ifndef APP_CLIENTS_TUI_THUMBNAIL_H
#define APP_CLIENTS_TUI_THUMBNAIL_H

/* A picture preview reduced to terminal cells: each cell is a half block
 * with a top and bottom colour (xterm-256 indexes). */
typedef struct Thumbnail {
    int    cols;
    int    rows;
    short *top;       /* cols * rows */
    short *bottom;    /* cols * rows */
} Thumbnail;

#endif
