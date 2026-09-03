#ifndef PARSER_H
#define PARSER_H

#include "config.h"
#include "history.h"

typedef struct {
    config_t *config;
    history_t *history;
} parser_global_context_t;

void parser_export_commit(void *global_context, void *local_context);

#endif
