#ifndef APP_CORE_IMPORT_VERDICT_H
#define APP_CORE_IMPORT_VERDICT_H

/* Whether a package of a moved account may be brought in, and if not, why. */
typedef enum ImportVerdict {
    IMPORT_OK = 0,
    IMPORT_BAD_MANIFEST,        /* a format this tawk does not know */
    IMPORT_NEWER_SCHEMA,        /* made by a newer tawk than this one */
    IMPORT_NO_ROOM              /* this tawk already holds as many accounts as it can */
} ImportVerdict;

/* The refusal in plain words; "" for IMPORT_OK. */
const char *import_verdict_reason(ImportVerdict verdict);

#endif
