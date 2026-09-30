#include "core/status_kind.h"

static const char *const NAMES[STATUS_KIND_COUNT]  = { "text", "image", "video", "link" };
static const char *const LABELS[STATUS_KIND_COUNT] = { "Text", "Photo", "Video", "Link" };

static int valid(StatusKind kind) { return kind >= 0 && kind < STATUS_KIND_COUNT; }

const char *status_kind_name(StatusKind kind) { return valid(kind) ? NAMES[kind] : NAMES[STATUS_KIND_TEXT]; }
const char *status_kind_label(StatusKind kind) { return valid(kind) ? LABELS[kind] : LABELS[STATUS_KIND_TEXT]; }
int status_kind_has_media(StatusKind kind) { return kind == STATUS_KIND_PHOTO || kind == STATUS_KIND_VIDEO; }
