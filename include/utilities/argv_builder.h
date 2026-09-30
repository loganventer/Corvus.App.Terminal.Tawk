#ifndef APP_UTILITIES_ARGV_BUILDER_H
#define APP_UTILITIES_ARGV_BUILDER_H

#include <stddef.h>

/* Builds a NULL-terminated argv whose strings live in caller storage. */
typedef struct ArgvBuilder {
    char  **argv;
    int     max_args;
    int     argc;
    char   *storage;
    size_t  size;
    size_t  used;
    int     failed;
} ArgvBuilder;

void argv_builder_init(ArgvBuilder *b, char **argv, int max_args, char *storage, size_t size);
void argv_builder_add(ArgvBuilder *b, const char *arg);
/* Returns argc, or -1 when anything overflowed. */
int  argv_builder_finish(ArgvBuilder *b);

#endif
