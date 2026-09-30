#ifndef APP_CLIENTS_TUI_CONTACT_ACTION_H
#define APP_CLIENTS_TUI_CONTACT_ACTION_H

/* What can be done from the contact details panel. */
typedef enum ContactAction {
    CONTACT_ACTION_VIEW_PHOTO = 0,
    CONTACT_ACTION_SEARCH,
    CONTACT_ACTION_OPTIONS,          /* mute, pin, archive, theme, tone */
    CONTACT_ACTION_SOFT_LOCK,
    CONTACT_ACTION_EXPORT,
    CONTACT_ACTION_EXPORT_MEDIA,
    CONTACT_ACTION_BLOCK,
    CONTACT_ACTION_UNBLOCK,
    CONTACT_ACTION_CLEAR,            /* delete the messages, keep the chat */
    CONTACT_ACTION_DELETE,           /* delete the whole chat */
    CONTACT_ACTION_COUNT
} ContactAction;

const char *contact_action_label(ContactAction action);
/* Actions that remove or lock things are confirmed in a dialog first. */
int         contact_action_needs_confirmation(ContactAction action);

#endif
