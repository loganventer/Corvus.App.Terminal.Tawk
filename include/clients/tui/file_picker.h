#ifndef APP_CLIENTS_TUI_FILE_PICKER_H
#define APP_CLIENTS_TUI_FILE_PICKER_H

#include "clients/tui/file_picker_result.h"
#include "clients/tui/ui_rect.h"
#include "utilities/file_entry.h"

#define FILE_PICKER_SHORTCUTS 8
#define FILE_PICKER_ROWS      256

/* A folder browser for choosing a file to send. Works the same on every
 * platform, so no desktop file dialog is needed. */
typedef struct FilePicker {
    int        open;
    char       dir[1024];
    FileEntry *entries;
    int        count;
    int        selected;          /* index into the filtered view; 0 is ".." */
    int        scroll;
    int        show_hidden;
    char       filter[64];
    char       picked[1300];

    char       shortcut_label[FILE_PICKER_SHORTCUTS][32];
    char       shortcut_path[FILE_PICKER_SHORTCUTS][512];
    int        shortcut_x[FILE_PICKER_SHORTCUTS + 1];
    int        shortcut_count;

    int        row_item[FILE_PICKER_ROWS];
    UiRect     last_rect;
} FilePicker;

void             file_picker_init(FilePicker *picker);
void             file_picker_dispose(FilePicker *picker);
/* Opens at `start_dir` (or the home folder when empty or missing). */
void             file_picker_open(FilePicker *picker, const char *start_dir);
void             file_picker_close(FilePicker *picker);
void             file_picker_render(FilePicker *picker, UiRect rect);
FilePickerResult file_picker_key(FilePicker *picker, int is_key_code, int ch);
FilePickerResult file_picker_click(FilePicker *picker, int y, int x);
void             file_picker_wheel(FilePicker *picker, int delta);

#endif
