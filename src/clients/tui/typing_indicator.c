#include "clients/tui/typing_indicator.h"
#include "clients/tui/tui_draw.h"
#include "clients/tui/tui_palette.h"
#include "utilities/str_util.h"
#include "utilities/utf8_text.h"

#include <string.h>

#define DOT "\xE2\x97\x8F"      /* ● */

void typing_indicator_draw(int y, int x, int width, const char *text, int phase) {
    if (!text || !text[0] || width < 8) return;
    char label[96];
    str_copy(label, sizeof(label), text);
    size_t n = strlen(label);
    if (n >= 3 && strcmp(label + n - 3, "\xE2\x80\xA6") == 0) label[n - 3] = '\0';   /* the dots say it */
    int bubble = tui_palette_conversation_attr(THEME_SLOT_BUBBLE_THEM);
    int cols = utf8_columns(label) + 9;                 /* " label ● ● ● " */
    if (cols > width) cols = width;
    tui_fill((UiRect){ y, x, 1, cols }, bubble);
    int used = tui_text(y, x + 1, cols - 8, label, bubble);
    int dx = x + 1 + used + 1;
    for (int i = 0; i < 3; i++) {                        /* one dot lit at a time, left to right */
        int lit = ((phase % 4) + 4) % 4 == i;
        tui_text(y, dx + i * 2, 1, DOT, lit ? tui_palette_conversation_attr(THEME_SLOT_OK) | ATTR_BOLD : bubble | ATTR_DIM);
    }
}
