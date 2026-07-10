#ifndef PARSER_H
#define PARSER_H

#include <tree_sitter/api.h>

#include "reader.h"
#include "repository.h"

typedef struct {
    TSParser *ts_parser;
} parser_t;

void parser_init(parser_t *parser);

void parser_destroy(parser_t *parser);

void parser_parse(parser_t *parser, file_t *file, const buffer_t *buffer);

#endif
