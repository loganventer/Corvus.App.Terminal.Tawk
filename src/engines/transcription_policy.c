#include "engines/transcription_policy.h"

int transcription_policy_allows(const Chat *chat, const ChatPrefs *prefs) {
    if (!chat || chat->is_locked > 0 || chat->is_private) return 0;
    return !(prefs && prefs->transcribe_off);
}
