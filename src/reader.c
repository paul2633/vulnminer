#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "utils.h"

void reader_local(const file_t *file, buffer_t *buffer) {
    FILE *f = fopen(file->absolute_path, "rb");
    exit_if(f == NULL, "fopen");

    exit_if(fseek(f, 0, SEEK_END) != 0, "fseek");
    long size = ftell(f);
    exit_if(size < 0, "ftell");

    rewind(f);

    buffer->size = (size_t)size;
    buffer->data = malloc(buffer->size + 1);
    exit_if(buffer->data == NULL, "malloc");

    exit_if(fread(buffer->data, 1, buffer->size, f) != buffer->size, "fread");

    buffer->data[buffer->size] = '\0';

    exit_if(fclose(f) == EOF, "fclose");
}
