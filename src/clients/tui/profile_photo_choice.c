#include "clients/tui/profile_photo_choice.h"
#include "core/icon_glyphs.h"

const char *profile_photo_choice_label(ProfilePhotoChoice choice) {
    switch (choice) {
        case PROFILE_PHOTO_CHOOSE_FILE: return ICON_FILE "  Choose a file";
        case PROFILE_PHOTO_TAKE_PHOTO:  return "\xF0\x9F\x93\xB7  Take a photo";            /* 📷 */
        case PROFILE_PHOTO_PASTE:       return "\xF0\x9F\x93\x8B  Paste a picture";         /* 📋 */
        case PROFILE_PHOTO_VIEW:        return "\xF0\x9F\x94\x8D  View full size";          /* 🔍 */
        case PROFILE_PHOTO_REMOVE:      return "\xE2\x9D\x8C  Remove photo";                /* ❌ */
        default:                        return "";
    }
}
