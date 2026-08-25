#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "exporter.h"
#include "github/github.h"
#include "parser.h"
#include "utils.h"

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

static void parser_find_functions(TSNode node, const char *file_content, dataset_file_t *file) {
    if (strcmp(ts_node_type(node), "function_definition") == 0) {
        char *function_name = get_function_name(node, file_content);
        dataset_file_add_before_function(file, function_name, ts_node_start_byte(node), ts_node_end_byte(node));
        free(function_name);
        return;
    }

    uint32_t child_count = ts_node_named_child_count(node);
    for (uint32_t i = 0; i < child_count; i++)
        parser_find_functions(ts_node_named_child(node, i), file_content, file);
}

static void parser_parse_file(parser_t *parser, dataset_file_t *file) {
    parser_set_language(parser, file->path);

    TSTree *tree = ts_parser_parse_string(parser->ts_parser, NULL, file->before, file->before_size);
    EXIT_IF(tree == NULL, "ts_parser_parse_string");

    parser_find_functions(ts_tree_root_node(tree), file->before, file);

    ts_tree_delete(tree);

    tree = ts_parser_parse_string(parser->ts_parser, NULL, file->after, file->after_size);
    EXIT_IF(tree == NULL, "ts_parser_parse_string");

    parser_find_functions(ts_tree_root_node(tree), file->after, file);

    ts_tree_delete(tree);
}

void parse_and_export_commit(void *global_context, void *local_context) {
    parser_global_context_t *global = global_context;
    const config_t *config = global->config;
    history_t *history = global->history;

    dataset_entry_t *entry = local_context;

    char *prefix = commit_to_display(entry->cwe_id, entry->cve_id, entry->repo_name, entry->commit_hash);
    unsigned line_number = history_add_line(history, history->parsing_section, prefix, "parsing...");
    free(prefix);

    parser_t parser;
    parser_init(&parser);

    for (unsigned i = 0; i < entry->files_count; i++)
        if (strcmp(entry->files[i]->status, "modified") == 0)
            parser_parse_file(&parser, entry->files[i]);

    parser_destroy(&parser);

    if (!exporter_export_commit(config->export_folder_path, entry))
        history_update_line(history, history->parsing_section, line_number, "filename already exists", true);
    else
        history_update_line(history, history->parsing_section, line_number, "complete", true);

    dataset_entry_destroy(entry);
}
