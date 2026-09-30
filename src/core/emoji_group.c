#include "core/emoji_group.h"

static const char *const LABELS[EMOJI_GROUP_COUNT] = {
    "Smileys", "People", "Animals", "Food", "Travel", "Activities", "Objects", "Symbols", "Flags"
};

static const char *const ICONS[EMOJI_GROUP_COUNT] = {
    "\xF0\x9F\x98\x80", "\xF0\x9F\x91\x8B", "\xF0\x9F\x90\xBB", "\xF0\x9F\x8D\x94", "\xF0\x9F\x9A\x97",
    "\xE2\x9A\xBD", "\xF0\x9F\x92\xA1", "\xE2\x9D\xA4", "\xF0\x9F\x8F\x81"
};

const char *emoji_group_label(EmojiGroup g) { return (g >= 0 && g < EMOJI_GROUP_COUNT) ? LABELS[g] : ""; }
const char *emoji_group_icon(EmojiGroup g)  { return (g >= 0 && g < EMOJI_GROUP_COUNT) ? ICONS[g] : ""; }
