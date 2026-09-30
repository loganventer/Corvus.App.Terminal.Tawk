#include "clients/tui/profile_view_dialog.h"
#include "clients/tui/portrait_view.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/str_util.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#define WIDTH      60
#define PORTRAIT_W 16
#define PORTRAIT_H 8
#define PENCIL     "\xE2\x9C\x8E"   /* ✎ */

void profile_view_dialog_open(ProfileViewDialog *d) {
    memset(d, 0, sizeof(*d));
    d->selected = PROFILE_FIELD_NAME;
    d->open = 1;
}

PopupResult profile_view_dialog_key(ProfileViewDialog *d, int is_key, int ch) {
    if (!is_key && (ch == 27 || ch == 'q')) { d->open = 0; return POPUP_CLOSED; }
    if (is_key && ch == KEY_UP && d->selected > 0) d->selected--;
    else if (is_key && ch == KEY_DOWN && d->selected < PROFILE_FIELD_COUNT - 1) d->selected++;
    else if ((!is_key && (ch == '\n' || ch == '\r' || ch == ' ')) || (is_key && (ch == KEY_ENTER || ch == KEY_RIGHT))) return POPUP_CHOSEN;
    else return POPUP_NONE;
    return POPUP_CHANGED;
}

PopupResult profile_view_dialog_click(ProfileViewDialog *d, int y, int x) {
    if (!ui_rect_contains(d->last_rect, y, x)) { d->open = 0; return POPUP_CLOSED; }
    if (ui_rect_contains(d->portrait_rect, y, x)) { d->selected = PROFILE_FIELD_PICTURE; return POPUP_CHOSEN; }
    for (int i = 0; i < PROFILE_FIELD_COUNT; i++) {
        if (ui_rect_contains(d->rows[i], y, x)) { d->selected = i; return POPUP_CHOSEN; }
    }
    return POPUP_NONE;
}

ProfileField profile_view_dialog_choice(const ProfileViewDialog *d) { return (ProfileField)d->selected; }

/* "+27821234567" from "27821234567@s.whatsapp.net". */
static void phone_of(const char *jid, char *out, size_t size) {
    out[0] = '\0';
    if (!jid || !*jid) return;
    const char *at = strchr(jid, '@');
    size_t n = at ? (size_t)(at - jid) : strlen(jid);
    const char *colon = memchr(jid, ':', n);             /* a device suffix, 2782...:12 */
    if (colon) n = (size_t)(colon - jid);
    snprintf(out, size, "+%.*s", (int)n, jid);
}

/* One changeable row: a dim label, the value in bold, and a pencil. */
static void draw_row(ProfileViewDialog *d, ProfileField field, int y, UiRect box, const char *label, const char *value,
                     int busy, int base) {
    int selected = d->selected == (int)field;
    int attr = selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) : base;
    UiRect row = { y, box.x + 1, 2, box.w - 2 };
    tui_fill(row, attr);
    tui_text(y, box.x + 3, box.w - 6, label, attr | ATTR_DIM);
    int cols = box.w - 10;
    tui_text(y + 1, box.x + 3, cols, value && *value ? value : "\xE2\x80\x94", attr | ATTR_BOLD);   /* — when empty */
    tui_text(y + 1, box.x + box.w - 5, 2, busy ? "\xE2\x80\xA6" : PENCIL, attr | tui_palette_attr(THEME_SLOT_ACCENT));
    d->rows[field] = row;
}

void profile_view_dialog_render(ProfileViewDialog *d, UiRect a, const ProfileViewModel *m, ThumbnailCache *thumbs) {
    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = PORTRAIT_H + 14;
    if (h > a.h) h = a.h;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    d->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_fill(box, base);
    tui_box(box, "Profile", tui_palette_attr(THEME_SLOT_BORDER));

    UiRect pic = { box.y + 1, box.x + (box.w - PORTRAIT_W) / 2, PORTRAIT_H, PORTRAIT_W };
    ImagePlacement unused;
    portrait_draw(pic, m->jid, m->name, m->picture, thumbs, 0, &unused);   /* half blocks: a dialog floats over it */
    d->portrait_rect = pic;

    int y = pic.y + pic.h + 1;
    draw_row(d, PROFILE_FIELD_NAME, y, box, "Name", m->name, m->busy[PROFILE_FIELD_NAME], base);
    y += 3;
    draw_row(d, PROFILE_FIELD_ABOUT, y, box, "About", m->about, m->busy[PROFILE_FIELD_ABOUT], base);
    y += 3;
    char phone[64];
    phone_of(m->jid, phone, sizeof(phone));
    tui_text(y, box.x + 3, box.w - 6, "Phone", base | ATTR_DIM);
    tui_text(y + 1, box.x + 3, box.w - 6, phone, base);
    y += 3;
    draw_row(d, PROFILE_FIELD_PICTURE, y, box, "Photo",
             m->picture ? "Change or remove your photo" : "Add a profile photo", m->busy[PROFILE_FIELD_PICTURE], base);
    tui_text_center(box.y + box.h - 1, box.x, box.w, " \xE2\x86\x91\xE2\x86\x93 choose \xC2\xB7 Enter change \xC2\xB7 Esc close ",
                    tui_palette_attr(THEME_SLOT_BORDER));
}
