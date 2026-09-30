#include "clients/tui/incoming_call_view.h"
#include "clients/tui/portrait_view.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

IncomingCallChoice incoming_call_key(int is_key, int ch) {
    if (!is_key && (ch == 'd' || ch == 'D' || ch == 'n' || ch == 'N')) return INCOMING_CALL_DECLINE;
    if ((!is_key && (ch == 27 || ch == '\n' || ch == '\r' || ch == ' ')) || (is_key && ch == KEY_ENTER)) return INCOMING_CALL_DISMISS;
    return INCOMING_CALL_NONE;
}

IncomingCallChoice incoming_call_click(const IncomingCallView *v, int y, int x) {
    if (ui_rect_contains(v->decline_button, y, x)) return INCOMING_CALL_DECLINE;
    if (ui_rect_contains(v->dismiss_button, y, x)) return INCOMING_CALL_DISMISS;
    return INCOMING_CALL_NONE;
}

int incoming_call_render(IncomingCallView *v, UiRect area, const char *jid, const char *name, const char *picture,
                         ThumbnailCache *thumbs, int pixel_images, int64_t ringing_ms, ImagePlacement *placement) {
    int w = area.w < 44 ? area.w : 44, h = area.h < 15 ? area.h : 15;
    UiRect box = { area.y + (area.h - h) / 2, area.x + (area.w - w) / 2, h, w };
    v->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE), ok = tui_palette_attr(THEME_SLOT_OK) | ATTR_BOLD;
    int pulse = (ringing_ms / 500) % 2 == 0;
    tui_fill(box, base);
    tui_box(box, " \xF0\x9F\x93\x9E Incoming voice call ", pulse ? ok | ATTR_REVERSE : ok);
    UiRect pic = { box.y + 2, box.x + (box.w - 12) / 2, 6, 12 };
    int placed = portrait_draw(pic, jid, name, picture, thumbs, pixel_images, placement);
    tui_text_center(pic.y + pic.h + 1, box.x, box.w, name, base | ATTR_BOLD);
    int secs = (int)(ringing_ms / 1000);
    char line[80];
    snprintf(line, sizeof(line), "Ringing %d:%02d " "\xC2\xB7" " answer it on your phone", secs / 60, secs % 60);
    tui_text_center(pic.y + pic.h + 2, box.x, box.w, line, tui_palette_attr(THEME_SLOT_DIM));
    const char *decline = "  Decline (d)  ", *dismiss = "  Answer on phone  ";
    int dw = (int)strlen(decline), aw = (int)strlen(dismiss);
    int bx = box.x + (box.w - dw - aw - 3) / 2, by = box.y + box.h - 2;
    tui_text(by, bx, dw, decline, tui_palette_attr(THEME_SLOT_WARN) | ATTR_REVERSE | ATTR_BOLD);
    tui_text(by, bx + dw + 3, aw, dismiss, tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD);
    v->decline_button = (UiRect){ by, bx, 1, dw };
    v->dismiss_button = (UiRect){ by, bx + dw + 3, 1, aw };
    return placed;
}
