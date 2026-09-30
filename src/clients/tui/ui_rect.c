#include "clients/tui/ui_rect.h"

int ui_rect_contains(UiRect r, int y, int x) {
    return r.h > 0 && r.w > 0 && y >= r.y && y < r.y + r.h && x >= r.x && x < r.x + r.w;
}
