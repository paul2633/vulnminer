#ifndef READER_H
#define READER_H

#include <stddef.h>

#include "repository.h"

typedef struct {
    char *data;
    size_t size;
} buffer_t;

void reader_local(const file_t *file, buffer_t *buffer);

#endif
