#include "clients/tui/attach_choice.h"
#include "core/icon_glyphs.h"

const char *attach_choice_label(AttachChoice choice) {
    switch (choice) {
        case ATTACH_CHOICE_PHOTO: return "\xF0\x9F\x93\xB7  Take a photo";   /* 📷 */
        case ATTACH_CHOICE_FILE:  return ICON_FILE "  Choose a file";
        default:                  return "";
    }
}
