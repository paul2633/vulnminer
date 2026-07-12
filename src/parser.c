#include <stddef.h>

#include "parser.h"
#include "reader.h"
#include "utils.h"

extern const TSLanguage *tree_sitter_c(void);

void parser_init(parser_t *parser) {
    parser->ts_parser = ts_parser_new();
    exit_if(parser->ts_parser == NULL, __func__, "ts_parser_new");
    exit_if(!ts_parser_set_language(parser->ts_parser, tree_sitter_c()), __func__, "ts_parser_set_language");
}

void parser_destroy(parser_t *parser) { ts_parser_delete(parser->ts_parser); }

void parser_parse(parser_t *parser, file_t *file, const buffer_t *buffer) {
    (void)parser;

    file->line_count = 0;

    for (size_t i = 0; i < buffer->size; i++) {
        if (buffer->data[i] == '\n')
            file->line_count++;
    }

    if (buffer->size > 0 && buffer->data[buffer->size - 1] != '\n')
        file->line_count++;
}
