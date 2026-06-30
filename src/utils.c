#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

void exit_if(int condition, const char *msg) {
    if (condition) {
        fprintf(stderr, "%s\n", msg);
        exit(EXIT_FAILURE);
    }
}