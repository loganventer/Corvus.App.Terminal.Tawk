#ifndef APP_CORE_ATTACHMENT_INFO_H
#define APP_CORE_ATTACHMENT_INFO_H

/* What sending a file would send, for the person asked to allow it. */
typedef struct AttachmentInfo {
    char        name[256];       /* the file's name, without its folder */
    const char *kind;            /* image, video, audio or document */
    long long   size_bytes;
} AttachmentInfo;

#endif
