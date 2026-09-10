#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tree_sitter/api.h>

#include "dataset.h"
#include "parser.h"
#include "utils.h"

typedef struct {
    TSParser *ts_parser;
} parser_t;

extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_cpp(void);

static void parser_init(parser_t *parser) {
    parser->ts_parser = ts_parser_new();
    EXIT_IF(parser->ts_parser == NULL, "ts_parser_new");
}

static void parser_destroy(parser_t *parser) { ts_parser_delete(parser->ts_parser); }

static void parser_set_language(parser_t *parser, const char *path) {
    const char *ext = strrchr(path, '.');

    EXIT_IF(ext == NULL, "invalid extension");

    if (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0)
        EXIT_IF(!ts_parser_set_language(parser->ts_parser, tree_sitter_c()), "ts_parser_set_language");

    else if (strcmp(ext, ".cpp") == 0 || strcmp(ext, ".hpp") == 0)
        EXIT_IF(!ts_parser_set_language(parser->ts_parser, tree_sitter_cpp()), "ts_parser_set_language");

    else
        EXIT_IF(true, "invalid extension %s", ext);
}

static TSNode find_identifier(TSNode node) {
    if (strcmp(ts_node_type(node), "identifier") == 0)
        return node;

    uint32_t child_count = ts_node_named_child_count(node);

    for (uint32_t i = 0; i < child_count; i++) {
        TSNode child = ts_node_named_child(node, i);
        TSNode result = find_identifier(child);

        if (!ts_node_is_null(result))
            return result;
    }

    return (TSNode){0};
}

static char *get_function_name(TSNode function_node, const char *content) {
    TSNode declarator = ts_node_child_by_field_name(function_node, "declarator", strlen("declarator"));

    if (ts_node_is_null(declarator))
        return NULL;

    TSNode identifier = find_identifier(declarator);

    if (ts_node_is_null(identifier))
        return NULL;

    uint32_t start = ts_node_start_byte(identifier), end = ts_node_end_byte(identifier);
    size_t size = end - start;

    char *name = malloc(size + 1);
    EXIT_IF(name == NULL, "malloc");

    memcpy(name, content + start, size);
    name[size] = '\0';

    return name;
}

static void parser_find_functions(TSNode node, const char *file_content, dataset_file_t *file, bool before) {
    if (strcmp(ts_node_type(node), "function_definition") == 0) {
        char *function_name = get_function_name(node, file_content);

        if (function_name == NULL)
            return;

        if (before)
            add_before_function(file, function_name, ts_node_start_byte(node), ts_node_end_byte(node));
        else
            add_after_function(file, function_name, ts_node_start_byte(node), ts_node_end_byte(node));

        free(function_name);
        return;
    }

    uint32_t child_count = ts_node_named_child_count(node);
    for (uint32_t i = 0; i < child_count; i++)
        parser_find_functions(ts_node_named_child(node, i), file_content, file, before);
}

static void parser_parse_file(parser_t *parser, dataset_file_t *file) {
    if (file->previous_path != NULL) {
        TSTree *tree = ts_parser_parse_string(parser->ts_parser, NULL, file->before, file->before_size);
        EXIT_IF(tree == NULL, "ts_parser_parse_string");

        parser_find_functions(ts_tree_root_node(tree), file->before, file, true);

        ts_tree_delete(tree);
    }

    if (file->path != NULL) {
        TSTree *tree = ts_parser_parse_string(parser->ts_parser, NULL, file->after, file->after_size);
        EXIT_IF(tree == NULL, "ts_parser_parse_string");

        parser_find_functions(ts_tree_root_node(tree), file->after, file, false);

        ts_tree_delete(tree);
    }
}

void parser_parse_commit(dataset_entry_t *entry) {
    parser_t parser;
    parser_init(&parser);

    for (unsigned i = 0; i < entry->files_count; i++) {
        dataset_file_t *file = entry->files[i];

        if (file->previous_path != NULL)
            parser_set_language(&parser, file->previous_path);
        else if (file->path != NULL)
            parser_set_language(&parser, file->path);

        parser_parse_file(&parser, file);
    }

    parser_destroy(&parser);
}
