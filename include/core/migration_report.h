#ifndef APP_CORE_MIGRATION_REPORT_H
#define APP_CORE_MIGRATION_REPORT_H

/* What a move of an account carried, for the person to read. */
typedef struct MigrationReport {
    int messages;
    int chats;
    int contacts;
    int other_rows;             /* receipts, reactions, statuses, transcripts and the rest */
    int scheduled_left_behind;  /* messages waiting for a time, which stay where they were */
    int media_files;
    int media_missing;          /* files the messages name that were no longer on disk */
} MigrationReport;

#endif
