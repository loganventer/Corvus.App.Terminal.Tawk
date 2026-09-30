#ifndef APP_CLIENTS_TUI_IMAGE_PLACEMENT_H
#define APP_CLIENTS_TUI_IMAGE_PLACEMENT_H

/* Where a photo sits on screen, left blank by the view for a pixel image. */
typedef struct ImagePlacement {
    int  message;       /* index into the message array */
    int  y, x;          /* top-left cell */
    int  cols, rows;
    int  attr;          /* background under it; a change means it was repainted */
    int  source;        /* MediaPictureSource: a sharper one means redraw */
    int  page;          /* PDFs: page shown */
    int  plain;         /* drawn without the document badge */
    int  round;         /* a portrait cut to a circle */
    char path[600];     /* file or poster for those sources */
    char id[64];        /* message id */
} ImagePlacement;

#endif
