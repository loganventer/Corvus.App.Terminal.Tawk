#ifndef APP_CLIENTS_TUI_AGENTS_VIEW_H
#define APP_CLIENTS_TUI_AGENTS_VIEW_H

/* The four parts of the Agents tab, on keys 1 to 4. */
typedef enum AgentsView {
    AGENTS_VIEW_QUEUE = 0,        /* requests waiting for you */
    AGENTS_VIEW_AGENTS,           /* programs connected now */
    AGENTS_VIEW_LOG,              /* what they did */
    AGENTS_VIEW_PERMISSIONS,      /* what they may do */
    AGENTS_VIEW_COUNT
} AgentsView;

#endif
