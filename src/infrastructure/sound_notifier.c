#include "infrastructure/sound_notifier.h"
#include "utilities/path_util.h"

#include <stdlib.h>
#include <string.h>

typedef struct SoundNotifier {
    IAudioPlayer   *player;
    const Settings *settings;
} SoundNotifier;

static void sound_notify(INotifier *self, const Notification *n) {
    SoundNotifier *sn = self->ctx;
    if (!sn->settings->sound || strcmp(n->tone, "none") == 0) return;
    const char *file = (n->tone[0] && path_is_regular_file(n->tone)) ? n->tone : sn->settings->sound_file;
    if (path_is_regular_file(file)) sn->player->play(sn->player, file);
}

static void sound_destroy(INotifier *self) {
    free(self->ctx);
    free(self);
}

INotifier *sound_notifier_create(IAudioPlayer *player, const Settings *settings) {
    INotifier *n = calloc(1, sizeof(*n));
    SoundNotifier *sn = calloc(1, sizeof(*sn));
    if (!n || !sn) { free(n); free(sn); return NULL; }
    sn->player = player;
    sn->settings = settings;
    n->ctx = sn;
    n->notify = sound_notify;
    n->destroy = sound_destroy;
    return n;
}
