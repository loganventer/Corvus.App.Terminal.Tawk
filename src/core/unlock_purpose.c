#include "core/unlock_purpose.h"

const char *unlock_purpose_reason(UnlockPurpose purpose) {
    switch (purpose) {
    case UNLOCK_FOR_OPEN:            return "open a private chat";
    case UNLOCK_FOR_MAKE_PUBLIC:     return "make a private chat public";
    case UNLOCK_FOR_EXPORT:          return "export a private chat";
    case UNLOCK_FOR_PASSWORD_CHANGE: return "change the lock password for private chats";
    case UNLOCK_PURPOSE_COUNT:       break;
    }
    return "unlock a private chat";
}
