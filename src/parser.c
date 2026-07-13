#include <stddef.h>
#include <stdio.h>
#include <string.h>

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

static void json_write_string(FILE *f, const char *s, size_t len) {
    fputc('"', f);

    for (size_t i = 0; i < len; i++) {
        switch (s[i]) {
        case '"':
            fputs("\\\"", f);
            break;

        case '\\':
            fputs("\\\\", f);
            break;

        case '\n':
            fputs("\\n", f);
            break;

        case '\r':
            fputs("\\r", f);
            break;

        case '\t':
            fputs("\\t", f);
            break;

        default:
            if ((unsigned char)s[i] < 0x20)
                fprintf(f, "\\u%04x", (unsigned char)s[i]);
            else
                fputc(s[i], f);
        }
    }

    fputc('"', f);
}

static void parse_lines(const buffer_t *source, FILE *f, size_t start_line, size_t end_line) {
    const char *p = source->data;
    size_t line = 1;

    while (line < start_line) {
        if (*p++ == '\n')
            line++;
    }

    while (line <= end_line) {

        const char *begin = p;

        while (*p != '\0' && *p != '\n')
            p++;

        json_write(f, 6, "{\n");
        json_write(f, 7, "\"number\": %zu,\n", line);
        //json_write(f, 7, "\"text\": \"%.*s\"\n", (int)(p - begin), begin);

        json_write(f, 7, "\"text\": ");
        json_write_string(f, begin, (int)(p - begin));
        json_write(f, 0, "\n");

        if (line < end_line)
            json_write(f, 6, "},\n");
        else
            json_write(f, 6, "}\n");

        if (*p == '\n')
            p++;

        line++;
    }
}

static void parse_functions(const buffer_t *source, config_t *config, TSNode node, FILE *f, bool *first) {

    if (strcmp(ts_node_type(node), "function_definition") == 0) {

        TSPoint start = ts_node_start_point(node);
        TSPoint end   = ts_node_end_point(node);

        size_t start_line = start.row + 1;
        size_t end_line   = end.row + 1;

        if (*first) {
            json_write(f, 0, "\n");
            *first = false;
        } else {
            json_write(f, 0, ",\n");
            
        }

        json_write(f, 4, "{\n");
        json_write(f, 5, "\"line_count\": %zu,\n", end_line - start_line + 1);
        json_write(f, 5, "\"start_line\": %zu,\n", start_line);

        if (config->granularity == GRANULARITY_LINE) {
            json_write(f, 5, "\"end_line\": %zu,\n", end_line);
            json_write(f, 5, "\"lines\": [\n");
            parse_lines(source, f, start_line, end_line);
            json_write(f, 5, "]\n");
        }

        else {
            json_write(f, 5, "\"end_line\": %zu\n", end_line);
        }


        json_write(f, 4, "}");
    }

    uint32_t child_count = ts_node_child_count(node);

    for (uint32_t i = 0; i < child_count; i++)
        parse_functions(source, config, ts_node_child(node, i), f, first);
}

void parser_parse(config_t *config, parser_t *parser, file_t *file, const buffer_t *source, buffer_t *json) {
    
    TSTree *tree = ts_parser_parse_string(parser->ts_parser, NULL, source->data, source->size);
    exit_if(tree == NULL, __func__, "ts_parser_parse_string");

    TSNode root = ts_tree_root_node(tree);
    TSPoint end = ts_node_end_point(root);

    FILE *f = open_memstream(&json->data, &json->size);
    exit_if(f == NULL, __func__, "open_memstream");

    json_write(f, 2, "{\n");
    json_write(f, 3, "\"name\": \"%s\",\n", file->name);
    json_write(f, 3, "\"path\": \"%s\",\n", file->relative_path);

    if (config->granularity == GRANULARITY_FUNCTION || config->granularity == GRANULARITY_LINE) {
        json_write(f, 3, "\"line_count\": %zu,\n", end.row + 1);
        json_write(f, 3, "\"functions\": [");
        bool first = true;
        parse_functions(source, config, root, f, &first);
        if (first)
            json_write(f, 0, "]\n");
        else {
            json_write(f, 0, "\n");
            json_write(f, 3, "]\n");
        }
        
    }

    else {
        json_write(f, 3, "\"line_count\": %zu\n", end.row + 1);
    }

    json_write(f, 2, "}");

    exit_if(fclose(f) == EOF, __func__, "fclose");
    ts_tree_delete(tree);
}
