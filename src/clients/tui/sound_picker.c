#include "clients/tui/sound_picker.h"

#include <ncurses.h>
#include <string.h>

#define PAGE_STEP 10

static int clamp(int value, int count) {
    if (value >= count) value = count - 1;
    return value < 0 ? 0 : value;
}

static SoundPickerOutcome move_to(SoundPicker *p, ISoundCatalog *c, int target) {
    int to = clamp(target, c->count(c));
    if (to == p->index) return SOUND_PICKER_OPEN;
    p->index = to;
    return SOUND_PICKER_MOVED;
}

static SoundPickerOutcome close_picker(SoundPicker *p) {
    p->active = 0;
    return SOUND_PICKER_CLOSED;
}

static SoundPickerOutcome choose(SoundPicker *p, ISoundCatalog *c) {
    p->active = 0;
    return c->count(c) > 0 ? SOUND_PICKER_CHOSEN : SOUND_PICKER_CLOSED;
}

void sound_picker_begin(SoundPicker *p, ISoundCatalog *c, const char *current_path) {
    memset(p, 0, sizeof(*p));
    int at = c->index_of_path(c, current_path);
    p->index = at < 0 ? 0 : at;
    p->active = 1;
}

SoundPickerOutcome sound_picker_key(SoundPicker *p, ISoundCatalog *c, int is_key, int ch) {
    if (!p->active) return SOUND_PICKER_CLOSED;
    if (is_key) {
        switch (ch) {
            case KEY_UP:        return move_to(p, c, p->index - 1);
            case KEY_DOWN:      return move_to(p, c, p->index + 1);
            case KEY_PPAGE:     return move_to(p, c, p->index - PAGE_STEP);
            case KEY_NPAGE:     return move_to(p, c, p->index + PAGE_STEP);
            case KEY_HOME:      return move_to(p, c, 0);
            case KEY_END:       return move_to(p, c, c->count(c) - 1);
            case KEY_BACKSPACE: return close_picker(p);
            default:            return SOUND_PICKER_OPEN;
        }
    }
    if (ch == '\n' || ch == '\r' || ch == ' ') return choose(p, c);
    if (ch == 27 || ch == 'q' || ch == 127 || ch == 8) return close_picker(p);
    return SOUND_PICKER_OPEN;
}

SoundPickerOutcome sound_picker_wheel(SoundPicker *p, ISoundCatalog *c, int delta) {
    return p->active ? move_to(p, c, p->index + delta) : SOUND_PICKER_CLOSED;
}

SoundPickerOutcome sound_picker_click(SoundPicker *p, ISoundCatalog *c, int y) {
    if (!p->active) return SOUND_PICKER_CLOSED;
    int row = y - p->list_y;
    if (row < 0 || row >= p->list_h) return SOUND_PICKER_OPEN;
    int item = p->scroll + row;
    if (item >= c->count(c)) return SOUND_PICKER_OPEN;
    if (item == p->index) return choose(p, c);
    p->index = item;
    return SOUND_PICKER_MOVED;
}

const SoundOption *sound_picker_current(const SoundPicker *p, ISoundCatalog *c) {
    return c->at(c, p->index);
}
