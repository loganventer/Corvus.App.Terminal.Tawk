#ifndef APP_ENGINES_CONFIRMATION_TOKEN_H
#define APP_ENGINES_CONFIRMATION_TOKEN_H

#include <stddef.h>

/* 32 random hex digits from the OS CSPRNG, for confirming a destructive request. */
int confirmation_token_generate(char *out, size_t size);

#endif
