#ifndef EXPORTER_H
#define EXPORTER_H

#include <stdio.h>

#include "config.h"
#include "repository.h"

FILE *exporter_begin(repository_t *repo, config_t *config);

void exporter_end(FILE *f);

// void exporter_export(repository_t *repo, config_t *config);

#endif
