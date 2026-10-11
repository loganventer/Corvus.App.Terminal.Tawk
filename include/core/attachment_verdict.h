#ifndef APP_CORE_ATTACHMENT_VERDICT_H
#define APP_CORE_ATTACHMENT_VERDICT_H

/* Whether a file on this computer may be sent as an attachment, and if not, why. */
typedef enum AttachmentVerdict {
    ATTACHMENT_OK = 0,
    ATTACHMENT_NOT_FOUND,       /* no such file */
    ATTACHMENT_NOT_A_FILE,      /* a folder, device or socket */
    ATTACHMENT_EMPTY,           /* nothing in it */
    ATTACHMENT_TOO_BIG,         /* over the size an agent may send */
    ATTACHMENT_PROTECTED        /* tawk's own login or database, or a place that holds secrets */
} AttachmentVerdict;

/* The refusal in plain words, for an error; "" for ATTACHMENT_OK. */
const char *attachment_verdict_reason(AttachmentVerdict verdict);

#endif
