#include "clients/cli/cli_confirm.h"

#include <stdio.h>

int cli_confirm(const char *question, int assume_yes) {
    if (assume_yes) return 1;
    printf("%s [y/N] ", question);
    fflush(stdout);
    char answer[16];
    if (!fgets(answer, sizeof(answer), stdin)) return 0;
    return answer[0] == 'y' || answer[0] == 'Y';
}
