#ifndef APP_CONTRACTS_I_STATUS_LIKER_H
#define APP_CONTRACTS_I_STATUS_LIKER_H

/* Likes someone's status privately: the heart shows only in its author's
 * viewers list. Handed out by backends that can address a like to the
 * author alone; others hand out none, and a like then goes as a reply. */
typedef struct IStatusLiker {
    void *ctx;
    int  (*like)(struct IStatusLiker *self, const char *author_jid, const char *status_id, const char *emoji);
} IStatusLiker;

#endif
