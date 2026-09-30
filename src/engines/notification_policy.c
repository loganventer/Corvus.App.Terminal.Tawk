#include "engines/notification_policy.h"

int notification_policy_should_notify(const Settings *s, const Chat *chat,
                                      const Message *msg, int live, int chat_is_open) {
    if (!live || msg->from_me) return 0;
    if (!s->notifications || s->do_not_disturb) return 0;
    /* Being mentioned gets through a muted chat and silenced groups, as on the phone. */
    int mentioned = msg->mentions_me && s->mention_notifications;
    if (chat && chat->is_muted && !mentioned) return 0;
    if (chat && chat->is_group && !s->group_notifications && !mentioned) return 0;
    if (chat_is_open) return 0;
    return 1;
}
