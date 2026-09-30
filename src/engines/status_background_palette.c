#include "engines/status_background_palette.h"

typedef struct Colour {
    uint32_t    argb;
    const char *name;
} Colour;

/* The colours the WhatsApp apps cycle through for text statuses. */
static const Colour COLOURS[] = {
    { 0xFF7E90A3, "Slate" },
    { 0xFF25D366, "Green" },
    { 0xFF128C7E, "Teal" },
    { 0xFF34B7F1, "Sky blue" },
    { 0xFF5696FF, "Blue" },
    { 0xFF8294CA, "Lavender" },
    { 0xFFA62C71, "Plum" },
    { 0xFFC69FCC, "Lilac" },
    { 0xFFFF7B6B, "Coral" },
    { 0xFFF0B330, "Amber" },
    { 0xFF243640, "Night" },
    { 0xFF55624C, "Olive" },
};

#define COLOUR_COUNT ((int)(sizeof(COLOURS) / sizeof(COLOURS[0])))

static int wrap(int index) {
    int i = index % COLOUR_COUNT;
    return i < 0 ? i + COLOUR_COUNT : i;
}

int status_background_count(void) { return COLOUR_COUNT; }
uint32_t status_background_at(int index) { return COLOURS[wrap(index)].argb; }
const char *status_background_name(int index) { return COLOURS[wrap(index)].name; }
