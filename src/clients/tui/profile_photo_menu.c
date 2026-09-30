#include "clients/tui/profile_photo_menu.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"

#include <ncurses.h>
#include <string.h>

#define WIDTH 34

static int usable(const ProfilePhotoMenu *m, int item) {
    if (item == PROFILE_PHOTO_TAKE_PHOTO) return m->has_camera;
    if (item == PROFILE_PHOTO_VIEW || item == PROFILE_PHOTO_REMOVE) return m->has_photo;
    return 1;
}

static void step(ProfilePhotoMenu *m, int dir) {
    for (int i = m->selected + dir; i >= 0 && i < PROFILE_PHOTO_COUNT; i += dir) {
        if (usable(m, i)) { m->selected = i; return; }
    }
}

void profile_photo_menu_open(ProfilePhotoMenu *m, int has_camera, int has_photo) {
    memset(m, 0, sizeof(*m));
    m->has_camera = has_camera;
    m->has_photo = has_photo;
    m->selected = PROFILE_PHOTO_CHOOSE_FILE;
    m->open = 1;
}

PopupResult profile_photo_menu_key(ProfilePhotoMenu *m, int is_key, int ch) {
    if (!is_key && (ch == 27 || ch == 'q')) { m->open = 0; return POPUP_CLOSED; }
    if (is_key && ch == KEY_UP) step(m, -1);
    else if (is_key && ch == KEY_DOWN) step(m, 1);
    else if ((!is_key && (ch == '\n' || ch == '\r' || ch == ' ')) || (is_key && (ch == KEY_ENTER || ch == KEY_RIGHT))) {
        m->open = 0;
        return POPUP_CHOSEN;
    } else {
        return POPUP_NONE;
    }
    return POPUP_CHANGED;
}

PopupResult profile_photo_menu_click(ProfilePhotoMenu *m, int y, int x) {
    if (!ui_rect_contains(m->last_rect, y, x)) { m->open = 0; return POPUP_CLOSED; }
    int item = y - m->last_rect.y - 1;
    if (item < 0 || item >= PROFILE_PHOTO_COUNT || !usable(m, item)) return POPUP_NONE;
    m->selected = item;
    m->open = 0;
    return POPUP_CHOSEN;
}

ProfilePhotoChoice profile_photo_menu_choice(const ProfilePhotoMenu *m) { return (ProfilePhotoChoice)m->selected; }

void profile_photo_menu_render(ProfilePhotoMenu *m, UiRect a) {
    int w = a.w < WIDTH ? a.w : WIDTH;
    int h = PROFILE_PHOTO_COUNT + 2;
    if (h > a.h) h = a.h;
    UiRect box = { a.y + (a.h - h) / 2, a.x + (a.w - w) / 2, h, w };
    m->last_rect = box;
    int base = tui_palette_attr(THEME_SLOT_BASE);
    tui_box(box, "Profile photo", tui_palette_attr(THEME_SLOT_BORDER));
    tui_fill((UiRect){ box.y + 1, box.x + 1, box.h - 2, box.w - 2 }, base);
    for (int i = 0; i < PROFILE_PHOTO_COUNT && i < box.h - 2; i++) {
        int y = box.y + 1 + i;
        int attr = i == m->selected ? tui_palette_attr(THEME_SLOT_SIDEBAR_SELECTED) | ATTR_BOLD : base;
        if (!usable(m, i)) attr |= ATTR_DIM;
        tui_fill((UiRect){ y, box.x + 1, 1, box.w - 2 }, attr);
        int used = tui_text(y, box.x + 2, box.w - 4, profile_photo_choice_label((ProfilePhotoChoice)i), attr);
        if (i == PROFILE_PHOTO_TAKE_PHOTO && !m->has_camera) tui_text(y, box.x + 2 + used, box.w - 4 - used, " (no camera)", attr);
    }
}
