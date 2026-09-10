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

static dataset_function_t *find_function(dataset_function_t **functions, unsigned functions_count, const char *name) {
    if (name == NULL)
        return NULL;

    for (unsigned i = 0; i < functions_count; i++)
        if (strcmp(functions[i]->name, name) == 0)
            return functions[i];

    return NULL;
}

static void generate_json_function(yyjson_mut_doc *doc, yyjson_mut_val *functions, const char *name, const char *status, const char *previous_content,
                                   const char *content) {
    yyjson_mut_val *function = yyjson_mut_obj(doc);

    yyjson_mut_obj_add_str(doc, function, "name", name);
    yyjson_mut_obj_add_str(doc, function, "status", status);

    yyjson_mut_obj_add_str(doc, function, "content", content);
    yyjson_mut_obj_add_str(doc, function, "previous_content", previous_content);

    yyjson_mut_arr_add_val(functions, function);
}

static void generate_json_functions(yyjson_mut_doc *doc, yyjson_mut_val *yyjson_file, dataset_file_t *dataset_file) {
    yyjson_mut_val *functions = yyjson_mut_arr(doc);

    for (unsigned i = 0; i < dataset_file->before_functions_count; i++) {
        const dataset_function_t *before = dataset_file->before_functions[i];
        const dataset_function_t *after = find_function(dataset_file->after_functions, dataset_file->after_functions_count, before->name);

        if (after == NULL)
            generate_json_function(doc, functions, before->name, "removed", before->content, NULL);
        else if (strcmp(before->content, after->content) != 0)
            generate_json_function(doc, functions, before->name, "modified", before->content, after->content);
    }

    for (unsigned i = 0; i < dataset_file->after_functions_count; i++) {
        const dataset_function_t *after = dataset_file->after_functions[i];

        if (find_function(dataset_file->before_functions, dataset_file->before_functions_count, after->name) == NULL)
            generate_json_function(doc, functions, after->name, "added", NULL, after->content);
    }

    yyjson_mut_obj_add_val(doc, yyjson_file, "affected_functions", functions);
}

static void generate_json_file(yyjson_mut_doc *doc, yyjson_mut_val *files, dataset_file_t *dataset_file) {
    yyjson_mut_val *yyjson_file = yyjson_mut_obj(doc);

    yyjson_mut_obj_add_str(doc, yyjson_file, "status", dataset_file->status);

    yyjson_mut_obj_add_str(doc, yyjson_file, "path", dataset_file->path);
    yyjson_mut_obj_add_str(doc, yyjson_file, "previous_path", dataset_file->previous_path);

    yyjson_mut_obj_add_str(doc, yyjson_file, "content", dataset_file->after);
    yyjson_mut_obj_add_str(doc, yyjson_file, "previous_content", dataset_file->before);

    generate_json_functions(doc, yyjson_file, dataset_file);

    yyjson_mut_arr_add_val(files, yyjson_file);
}

static void generate_json_files(yyjson_mut_doc *doc, yyjson_mut_val *root, dataset_entry_t *entry) {
    yyjson_mut_val *files = yyjson_mut_arr(doc);

    for (unsigned i = 0; i < entry->files_count; i++)
        generate_json_file(doc, files, entry->files[i]);

    yyjson_mut_obj_add_val(doc, root, "commit_files", files);
}

static void generate_json_context_distances(yyjson_mut_doc *doc, yyjson_mut_val *yyjson_context_file, dataset_context_file_t *dataset_context_file) {
    yyjson_mut_val *context_distances = yyjson_mut_arr(doc);

    for (unsigned i = 0; i < dataset_context_file->distances_count; i++) {
        yyjson_mut_val *context_distance = yyjson_mut_obj(doc);

        yyjson_mut_obj_add_str(doc, context_distance, "path", dataset_context_file->distances[i]->path);
        yyjson_mut_obj_add_uint(doc, context_distance, "distance", dataset_context_file->distances[i]->distance);

        yyjson_mut_arr_add_val(context_distances, context_distance);
    }

    yyjson_mut_obj_add_val(doc, yyjson_context_file, "distances", context_distances);
}

static void generate_json_context_file(yyjson_mut_doc *doc, yyjson_mut_val *context_files, dataset_context_file_t *dataset_context_file) {
    yyjson_mut_val *yyjson_context_file = yyjson_mut_obj(doc);

    yyjson_mut_obj_add_str(doc, yyjson_context_file, "path", dataset_context_file->path);
    yyjson_mut_obj_add_str(doc, yyjson_context_file, "content", dataset_context_file->content);

    generate_json_context_distances(doc, yyjson_context_file, dataset_context_file);

    yyjson_mut_arr_add_val(context_files, yyjson_context_file);
}

static void generate_json_context_files(yyjson_mut_doc *doc, yyjson_mut_val *root, dataset_entry_t *entry) {
    yyjson_mut_val *context_files = yyjson_mut_arr(doc);

    for (unsigned i = 0; i < entry->context_files_count; i++)
        generate_json_context_file(doc, context_files, entry->context_files[i]);

    yyjson_mut_obj_add_val(doc, root, "context_files", context_files);
}

static char *generate_json(dataset_entry_t *entry, size_t *len) {
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    EXIT_IF(doc == NULL, "yyjson_mut_doc_new");

    yyjson_mut_val *root = yyjson_mut_obj(doc);
    EXIT_IF(root == NULL, "yyjson_mut_obj");

    yyjson_mut_doc_set_root(doc, root);

    generate_json_header(doc, root, entry);
    generate_json_files(doc, root, entry);
    generate_json_context_files(doc, root, entry);

    yyjson_write_err err = {0};
    char *json = yyjson_mut_write_opts(doc, YYJSON_WRITE_PRETTY | YYJSON_WRITE_ALLOW_INVALID_UNICODE, NULL, len, &err);
    EXIT_IF(json == NULL, "yyjson_mut_write: %s", err.msg);

    yyjson_mut_doc_free(doc);
    return json;
}

static void export_str(const char *str, size_t len, int fd) {
    FILE *f = fdopen(fd, "w");
    EXIT_IF(f == NULL, "fdopen");

    EXIT_IF(fwrite(str, 1, len, f) != len, "fwrite");
    EXIT_IF(fclose(f) == EOF, "fclose");
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

    // parser_parse_commit(entry);

    int fd = open(output_path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    free(output_path);

    if (fd == -1 && errno == EEXIST) {
        EXIT_IF(errno != EEXIST, "open");
        dataset_entry_destroy(entry);
        history_update_line(history, history->parsing_section, line_number, "filename already exists", true);
        return;
    }

    size_t len = 0;
    char *json = generate_json(entry, &len);

    export_str(json, len, fd);
    free(json);

    dataset_entry_destroy(entry);
    history_update_line(history, history->parsing_section, line_number, "complete", true);
}
