#ifndef APP_CLIENTS_TUI_UI_RECT_H
#define APP_CLIENTS_TUI_UI_RECT_H

/* A screen region in cells. */
typedef struct UiRect {
    int y;
    int x;
    int h;
    int w;
} UiRect;

int ui_rect_contains(UiRect rect, int y, int x);

#endif
