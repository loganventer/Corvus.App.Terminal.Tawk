#include "engines/receipt_details_builder.h"
#include "engines/played_time.h"
#include "utilities/clock_util.h"

#include <stdio.h>
#include <string.h>

#define TIME_BYTES 32

static void exact(int64_t at, char *out, size_t size) {
    if (at > 0) clock_format_exact(at, out, size);
    else snprintf(out, size, "not yet");
}

static void add(ReceiptDetails *d, const char *label, const char *time) {
    if (d->line_count >= RECEIPT_DETAILS_MAX_LINES) return;
    snprintf(d->lines[d->line_count++], RECEIPT_DETAILS_LINE_BYTES, "%s %s", label, time);
}

static void add_person(ReceiptDetails *d, const Receipt *r) {
    if (d->line_count >= RECEIPT_DETAILS_MAX_LINES) return;
    const char *name = r->name[0] ? r->name : r->jid;
    char t[TIME_BYTES];
    const char *what;
    int64_t at;
    if (r->played_at > 0)         { what = "played";    at = r->played_at; }
    else if (r->read_at > 0)      { what = "read";      at = r->read_at; }
    else                          { what = "delivered"; at = r->delivered_at; }
    exact(at, t, sizeof(t));
    snprintf(d->lines[d->line_count++], RECEIPT_DETAILS_LINE_BYTES, "%s: %s %s", name, what, t);
}

void receipt_details_build(const Message *m, const Receipt *r, int count, int group, ReceiptDetails *out) {
    memset(out, 0, sizeof(*out));
    if (!m || !m->from_me || m->deleted || !played_time_applies(m->type) || !r || count <= 0) return;

    char sent[TIME_BYTES], t[TIME_BYTES];
    exact(m->timestamp, sent, sizeof(sent));
    int64_t first = played_time_first(r, count);
    out->shown = 1;
    out->played = first > 0;
    add(out, "Sent", sent);

    if (!group) {
        exact(r[0].delivered_at ? r[0].delivered_at : (r[0].read_at ? r[0].read_at : r[0].played_at), t, sizeof(t));
        add(out, "Delivered", t);
        exact(r[0].read_at ? r[0].read_at : r[0].played_at, t, sizeof(t));
        add(out, "Read", t);
        exact(first, t, sizeof(t));
        add(out, "Played", t);
        if (first > 0) snprintf(out->head, sizeof(out->head), "Played %s", t);
        else snprintf(out->head, sizeof(out->head), "Not played yet");
        return;
    }

    int players = 0;
    for (int i = 0; i < count; i++) if (r[i].played_at > 0) players++;
    for (int i = 0; i < count; i++) add_person(out, &r[i]);
    if (first > 0) {
        exact(first, t, sizeof(t));
        snprintf(out->head, sizeof(out->head), "Played by %d of %d, first %s", players, count, t);
    } else {
        snprintf(out->head, sizeof(out->head), "Played by 0 of %d", count);
    }
    if (count > RECEIPT_DETAILS_MAX_LINES - 1 && out->line_count == RECEIPT_DETAILS_MAX_LINES) {
        snprintf(out->lines[RECEIPT_DETAILS_MAX_LINES - 1], RECEIPT_DETAILS_LINE_BYTES, "and %d more", count - (RECEIPT_DETAILS_MAX_LINES - 2));
    }
}
