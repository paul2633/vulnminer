#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tree_sitter/api.h>

#include "dataset.h"
#include "parser.h"
#include "utils.h"

typedef enum { BEFORE, AFTER } file_side;

extern const TSLanguage *tree_sitter_c(void);
extern const TSLanguage *tree_sitter_cpp(void);

static void parser_set_language(TSParser *ts_parser, const char *path) {
    const char *ext = strrchr(path, '.');
    EXIT_IF(ext == NULL, "invalid extension");

    if (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0) {
        EXIT_IF(!ts_parser_set_language(ts_parser, tree_sitter_c()), "ts_parser_set_language");
        return;
    }

    if (strcmp(ext, ".cpp") == 0 || strcmp(ext, ".hpp") == 0) {
        EXIT_IF(!ts_parser_set_language(ts_parser, tree_sitter_cpp()), "ts_parser_set_language");
        return;
    }

    EXIT_IF(true, "invalid extension %s", ext);
}

static char *node_to_string(TSNode node, const char *content) {
    uint32_t start = ts_node_start_byte(node);
    uint32_t end = ts_node_end_byte(node);
    size_t size = end - start;

    char *str = malloc(size + 1);
    EXIT_IF(str == NULL, "malloc");

    memcpy(str, content + start, size);
    str[size] = '\0';
    return str;
}

static TSNode find_identifier(TSNode node) {
    if (strcmp(ts_node_type(node), "identifier") == 0)
        return node;

    uint32_t child_count = ts_node_named_child_count(node);

    for (uint32_t i = 0; i < child_count; i++) {
        TSNode result = find_identifier(ts_node_named_child(node, i));
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

    return node_to_string(identifier, content);
}

static bool get_function_parameters_count(TSNode function_node, unsigned *parameters_count) {
    *parameters_count = 0;

    TSNode declarator = ts_node_child_by_field_name(function_node, "declarator", strlen("declarator"));
    if (ts_node_is_null(declarator))
        return false;

    TSNode parameter_list = ts_node_child_by_field_name(declarator, "parameters", strlen("parameters"));
    if (ts_node_is_null(parameter_list))
        return false;

    uint32_t count = ts_node_named_child_count(parameter_list);

    for (uint32_t i = 0; i < count; i++) {
        TSNode parameter = ts_node_named_child(parameter_list, i);

        if (strcmp(ts_node_type(parameter), "parameter_declaration") != 0)
            return false;

        *parameters_count += 1;
    }

    return true;
}

static void parser_find_functions(TSNode node, const char *file_content, dataset_file_t *file, file_side side) {
    if (strcmp(ts_node_type(node), "function_definition") == 0) {
        unsigned parameters_count = 0;
        if (!get_function_parameters_count(node, &parameters_count))
            return;

        char *function_name = get_function_name(node, file_content);
        if (function_name == NULL)
            return;

        if (side == BEFORE)
            add_before_function(file, function_name, parameters_count, ts_node_start_byte(node), ts_node_end_byte(node));
        else if (side == AFTER)
            add_after_function(file, function_name, parameters_count, ts_node_start_byte(node), ts_node_end_byte(node));

        free(function_name);
        return;
    }

    uint32_t child_count = ts_node_named_child_count(node);
    for (uint32_t i = 0; i < child_count; i++)
        parser_find_functions(ts_node_named_child(node, i), file_content, file, side);
}

static void parser_parse_file(TSParser *ts_parser, dataset_file_t *file) {
    if (file->previous_path != NULL) {
        TSTree *tree = ts_parser_parse_string(ts_parser, NULL, file->before, file->before_size);
        EXIT_IF(tree == NULL, "ts_parser_parse_string");
        parser_find_functions(ts_tree_root_node(tree), file->before, file, BEFORE);
        ts_tree_delete(tree);
    }

    if (file->path != NULL) {
        TSTree *tree = ts_parser_parse_string(ts_parser, NULL, file->after, file->after_size);
        EXIT_IF(tree == NULL, "ts_parser_parse_string");
        parser_find_functions(ts_tree_root_node(tree), file->after, file, AFTER);
        ts_tree_delete(tree);
    }
}

void parser_parse_commit(dataset_entry_t *entry) {
    TSParser *ts_parser = ts_parser_new();
    EXIT_IF(ts_parser == NULL, "ts_parser_new");

    for (unsigned i = 0; i < entry->files_count; i++) {
        dataset_file_t *file = entry->files[i];
        parser_set_language(ts_parser, file->path != NULL ? file->path : file->previous_path);
        parser_parse_file(ts_parser, file);
    }

    ts_parser_delete(ts_parser);
}
