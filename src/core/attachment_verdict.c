#include "core/attachment_verdict.h"

const char *attachment_verdict_reason(AttachmentVerdict v) {
    switch (v) {
        case ATTACHMENT_NOT_FOUND: return "No such file on this computer";
        case ATTACHMENT_NOT_A_FILE: return "That is not a plain file (a folder cannot be sent)";
        case ATTACHMENT_EMPTY: return "The file is empty";
        case ATTACHMENT_TOO_BIG: return "The file is larger than an agent may send";
        case ATTACHMENT_PROTECTED: return "That file is part of tawk's own data or holds secrets, and is never sent";
        default: return "";
    }
}
