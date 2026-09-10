#ifndef EXPORTER_H
#define EXPORTER_H

#include "config.h"
#include "history.h"

typedef struct {
    config_t *config;
    history_t *history;
} exporter_global_context_t;

void parse_and_export_commit(void *global_context, void *local_context);

#endif
