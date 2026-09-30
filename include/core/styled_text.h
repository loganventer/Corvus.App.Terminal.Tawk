#ifndef APP_CORE_STYLED_TEXT_H
#define APP_CORE_STYLED_TEXT_H

#include "core/styled_run.h"

/* Text as it is shown, with its formatting marks taken out, and the runs
 * that say how each part is drawn. The runs cover the text end to end. */
typedef struct StyledText {
    char      *text;
    StyledRun *runs;
    int        run_count;
} StyledText;

void styled_text_init(StyledText *styled);
void styled_text_dispose(StyledText *styled);
/* The style of the byte at `offset`. */
int  styled_text_style_at(const StyledText *styled, size_t offset);

#endif
