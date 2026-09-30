#ifndef APP_CORE_EMOJI_GROUP_H
#define APP_CORE_EMOJI_GROUP_H

/* Unicode emoji groups, in the order of emoji-test.txt. */
typedef enum EmojiGroup {
    EMOJI_GROUP_SMILEYS = 0,
    EMOJI_GROUP_PEOPLE,
    EMOJI_GROUP_ANIMALS,
    EMOJI_GROUP_FOOD,
    EMOJI_GROUP_TRAVEL,
    EMOJI_GROUP_ACTIVITIES,
    EMOJI_GROUP_OBJECTS,
    EMOJI_GROUP_SYMBOLS,
    EMOJI_GROUP_FLAGS,
    EMOJI_GROUP_COUNT
} EmojiGroup;

/* Short tab label, e.g. "Smileys". */
const char *emoji_group_label(EmojiGroup group);
/* A representative emoji for the tab. */
const char *emoji_group_icon(EmojiGroup group);

#endif
