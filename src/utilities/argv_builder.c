#include "utilities/argv_builder.h"

#include <string.h>

void argv_builder_init(ArgvBuilder *b, char **argv, int max_args, char *storage, size_t size) {
    b->argv = argv;
    b->max_args = max_args;
    b->argc = 0;
    b->storage = storage;
    b->size = size;
    b->used = 0;
    b->failed = 0;
}

void argv_builder_add(ArgvBuilder *b, const char *arg) {
    size_t len = strlen(arg) + 1;
    if (b->failed || b->argc >= b->max_args - 1 || b->used + len > b->size) { b->failed = 1; return; }
    memcpy(b->storage + b->used, arg, len);
    b->argv[b->argc++] = b->storage + b->used;
    b->used += len;
}

int argv_builder_finish(ArgvBuilder *b) {
    if (b->failed) return -1;
    b->argv[b->argc] = NULL;
    return b->argc;
}
