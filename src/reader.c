#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "utils.h"

void reader_local(const file_t *file, buffer_t *buffer) {
    FILE *f = fopen(file->absolute_path, "rb");
    exit_if(f == NULL, __func__, "fopen");

    exit_if(fseek(f, 0, SEEK_END) != 0, __func__, "fseek");
    long size = ftell(f);
    exit_if(size < 0, __func__, "ftell");
    buffer->size = (size_t)size;
    buffer->data = malloc(buffer->size + 1);
    exit_if(buffer->data == NULL, __func__, "malloc");

    exit_if(fseek(f, 0, SEEK_SET) != 0, __func__, "fseek");
    exit_if(fread(buffer->data, 1, buffer->size, f) != buffer->size, __func__, "fread");
    buffer->data[buffer->size] = '\0';

    exit_if(fclose(f) == EOF, __func__, "fclose");
}
