#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dataset.h"
#include "exporter.h"
#include "github/github.h"
#include "parser.h"
#include "utils.h"
#include "yyjson.h"

static void generate_json_header(yyjson_mut_doc *doc, yyjson_mut_val *root, dataset_entry_t *entry) {
    yyjson_mut_obj_add_uint(doc, root, "CWE", entry->cwe_id);

    yyjson_mut_obj_add_str(doc, root, "CVE", entry->cve_id);
    yyjson_mut_obj_add_str(doc, root, "CVE_published", entry->cve_published);
    yyjson_mut_obj_add_str(doc, root, "CVE_description", entry->cve_description);

    yyjson_mut_obj_add_str(doc, root, "repository", entry->repo_name);
    yyjson_mut_obj_add_str(doc, root, "commit_hash", entry->commit_hash);
    yyjson_mut_obj_add_str(doc, root, "commit_message", entry->commit_message);
}

static dataset_function_t *find_function_match(const dataset_function_t *function, dataset_function_t **functions_arr, unsigned functions_count) {
    for (unsigned i = 0; i < functions_count; i++) {
        if (strcmp(function->name, functions_arr[i]->name) != 0)
            continue;

        if (function->parameters_count != functions_arr[i]->parameters_count)
            continue;

        return functions_arr[i];
    }

    return NULL;
}

static void generate_json_function(yyjson_mut_doc *doc, yyjson_mut_val *yyjson_functions, const char *name, unsigned parameters_count, const char *status,
                                   const char *previous_content, const char *content) {
    yyjson_mut_val *yyjson_function = yyjson_mut_obj(doc);

    yyjson_mut_obj_add_str(doc, yyjson_function, "name", name);
    yyjson_mut_obj_add_uint(doc, yyjson_function, "parameters_number", parameters_count);
    yyjson_mut_obj_add_str(doc, yyjson_function, "status", status);

    yyjson_mut_obj_add_str(doc, yyjson_function, "content", content);
    yyjson_mut_obj_add_str(doc, yyjson_function, "previous_content", previous_content);

    yyjson_mut_arr_add_val(yyjson_functions, yyjson_function);
}

static void generate_json_functions(yyjson_mut_doc *doc, yyjson_mut_val *yyjson_file, dataset_file_t *file) {
    yyjson_mut_val *yyjson_functions = yyjson_mut_arr(doc);

    for (unsigned i = 0; i < file->before_functions_count; i++) {
        const dataset_function_t *before = file->before_functions[i];
        const dataset_function_t *after = find_function_match(before, file->after_functions, file->after_functions_count);

        if (after == NULL)
            generate_json_function(doc, yyjson_functions, before->name, before->parameters_count, "removed", before->content, NULL);
        else if (strcmp(before->content, after->content) != 0)
            generate_json_function(doc, yyjson_functions, before->name, before->parameters_count, "modified", before->content, after->content);
    }

    for (unsigned i = 0; i < file->after_functions_count; i++) {
        const dataset_function_t *after = file->after_functions[i];

        if (find_function_match(after, file->before_functions, file->before_functions_count) == NULL)
            generate_json_function(doc, yyjson_functions, after->name, after->parameters_count, "added", NULL, after->content);
    }

    yyjson_mut_obj_add_val(doc, yyjson_file, "affected_functions", yyjson_functions);
}

static void generate_json_files(yyjson_mut_doc *doc, yyjson_mut_val *root, dataset_entry_t *entry) {
    yyjson_mut_val *yyjson_files = yyjson_mut_arr(doc);

    for (unsigned i = 0; i < entry->files_count; i++) {
        yyjson_mut_val *yyjson_file = yyjson_mut_obj(doc);
        dataset_file_t *file = entry->files[i];

        yyjson_mut_obj_add_str(doc, yyjson_file, "status", file->status);

        yyjson_mut_obj_add_str(doc, yyjson_file, "path", file->path);
        yyjson_mut_obj_add_str(doc, yyjson_file, "previous_path", file->previous_path);

        yyjson_mut_obj_add_str(doc, yyjson_file, "content", file->after);
        yyjson_mut_obj_add_str(doc, yyjson_file, "previous_content", file->before);

        generate_json_functions(doc, yyjson_file, file);

        yyjson_mut_arr_add_val(yyjson_files, yyjson_file);
    }

    yyjson_mut_obj_add_val(doc, root, "commit_files", yyjson_files);
}

static void generate_json_context_distances(yyjson_mut_doc *doc, yyjson_mut_val *yyjson_context_file, dataset_context_file_t *context_file) {
    yyjson_mut_val *yyjson_context_distances = yyjson_mut_arr(doc);

    for (unsigned i = 0; i < context_file->distances_count; i++) {
        yyjson_mut_val *yyjson_context_distance = yyjson_mut_obj(doc);
        dataset_context_distance_t *context_distance = context_file->distances[i];

        yyjson_mut_obj_add_str(doc, yyjson_context_distance, "path", context_distance->path);
        yyjson_mut_obj_add_uint(doc, yyjson_context_distance, "distance", context_distance->distance);

        yyjson_mut_arr_add_val(yyjson_context_distances, yyjson_context_distance);
    }

    yyjson_mut_obj_add_val(doc, yyjson_context_file, "distances", yyjson_context_distances);
}

static void generate_json_context_files(yyjson_mut_doc *doc, yyjson_mut_val *root, dataset_entry_t *entry) {
    yyjson_mut_val *yyjson_context_files = yyjson_mut_arr(doc);

    for (unsigned i = 0; i < entry->context_files_count; i++) {
        yyjson_mut_val *yyjson_context_file = yyjson_mut_obj(doc);
        dataset_context_file_t *context_file = entry->context_files[i];

        yyjson_mut_obj_add_str(doc, yyjson_context_file, "path", context_file->path);
        yyjson_mut_obj_add_str(doc, yyjson_context_file, "content", context_file->content);

        generate_json_context_distances(doc, yyjson_context_file, context_file);

        yyjson_mut_arr_add_val(yyjson_context_files, yyjson_context_file);
    }

    yyjson_mut_obj_add_val(doc, root, "context_files", yyjson_context_files);
}

static char *generate_json(dataset_entry_t *entry, size_t *size) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    EXIT_IF(doc == NULL, "yyjson_mut_doc_new");

    yyjson_mut_val *root = yyjson_mut_obj(doc);
    EXIT_IF(root == NULL, "yyjson_mut_obj");

    yyjson_mut_doc_set_root(doc, root);

    generate_json_header(doc, root, entry);
    generate_json_files(doc, root, entry);
    generate_json_context_files(doc, root, entry);

    yyjson_write_err err = {0};
    char *json = yyjson_mut_write_opts(doc, YYJSON_WRITE_PRETTY | YYJSON_WRITE_ALLOW_INVALID_UNICODE, NULL, size, &err);
    EXIT_IF(json == NULL, "yyjson_mut_write: %s", err.msg);

    yyjson_mut_doc_free(doc);
    return json;
}

static bool export_str(const char *str, size_t len, const char *output_path) {
    int fd = open(output_path, O_WRONLY | O_CREAT | O_EXCL, 0644);

    if (fd == -1) {
        EXIT_IF(errno != EEXIST, "open");
        return false;
    }

    FILE *f = fdopen(fd, "w");
    EXIT_IF(f == NULL, "fdopen");

    EXIT_IF(fwrite(str, 1, len, f) != len, "fwrite");
    EXIT_IF(fclose(f) == EOF, "fclose");

    return true;
}

void parse_and_export_commit(void *global_context, void *local_context) {
    exporter_global_context_t *global = global_context;
    const config_t *config = global->config;
    history_t *history = global->history;

    dataset_entry_t *entry = local_context;

    char *prefix = commit_to_display(entry->cwe_id, entry->cve_id, entry->cve_published, entry->repo_name, entry->commit_hash);
    unsigned line_number = history_add_line(history, history->parsing_section, prefix, "parsing...");
    free(prefix);

    const char *repo_basename = strrchr(entry->repo_name, '/');
    repo_basename = repo_basename != NULL ? repo_basename + 1 : entry->repo_name;

    char *output_path = NULL;
    EXIT_IF(asprintf(&output_path, "%s/CWE-%u_%s_%s_%.8s.json", config->export_folder_path, entry->cwe_id, entry->cve_id, repo_basename, entry->commit_hash) ==
                -1,
            "asprintf");

    parser_parse_commit(entry);

    size_t size = 0;
    char *json = generate_json(entry, &size);

    bool export_successful = export_str(json, size, output_path);

    free(json);
    free(output_path);
    dataset_entry_destroy(entry);

    if (!export_successful) {
        history_update_line(history, history->parsing_section, line_number, "filename already exists", true);
        return;
    }

    history_update_line(history, history->parsing_section, line_number, "complete", true);
}
