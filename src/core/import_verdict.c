#include "core/import_verdict.h"

const char *import_verdict_reason(ImportVerdict v) {
    switch (v) {
        case IMPORT_BAD_MANIFEST: return "this is not an account package this tawk can read";
        case IMPORT_NEWER_SCHEMA: return "the package was made by a newer tawk than this one; update tawk first";
        case IMPORT_NO_ROOM: return "this tawk already holds as many accounts as it can; remove one first";
        default: return "";
    }
}
