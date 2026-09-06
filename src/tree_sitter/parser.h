#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <tree_sitter/api.h>

#include "config.h"
#include "dataset.h"
#include "history.h"

typedef struct {
    config_t *config;
    history_t *history;
} parser_global_context_t;

typedef struct {
    TSParser *ts_parser;
} parser_t;

void parse_and_export_commit(void *global_context, void *local_context);

#endif
