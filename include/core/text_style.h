#ifndef APP_CORE_TEXT_STYLE_H
#define APP_CORE_TEXT_STYLE_H

/* How a stretch of message text is drawn, as bits that combine. */
typedef enum TextStyle {
    TEXT_STYLE_PLAIN   = 0,
    TEXT_STYLE_BOLD    = 1 << 0,   /* *bold* */
    TEXT_STYLE_ITALIC  = 1 << 1,   /* _italic_ */
    TEXT_STYLE_STRIKE  = 1 << 2,   /* ~strikethrough~ */
    TEXT_STYLE_CODE    = 1 << 3,   /* `inline code` */
    TEXT_STYLE_BLOCK   = 1 << 4,   /* ```monospace block``` */
    TEXT_STYLE_QUOTE   = 1 << 5,   /* a "> " line */
    TEXT_STYLE_MENTION = 1 << 6    /* @Name */
} TextStyle;

#endif
