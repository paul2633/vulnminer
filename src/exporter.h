#ifndef EXPORTER_H
#define EXPORTER_H

#include <stdio.h>
#include <stdbool.h>

#include "config.h"
#include "reader.h"
#include "repository.h"

typedef struct {
    FILE *file;
    char *path;
    size_t remaining_files;
} exporter_t;

FILE *exporter_begin(repository_t *repo, config_t *config);

void exporter_export(FILE *f, const buffer_t *buffer, bool last);

void exporter_end(FILE *f);

#endif
