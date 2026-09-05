#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dataset.h"
#include "github/github.h"
#include "parser.h"
#include "utils.h"
#include "yyjson.h"

void parser_export_commit(void *global_context, void *local_context) {

    parser_global_context_t *global = global_context;
    config_t *config = global->config;
    history_t *history = global->history;

    dataset_entry_t *entry = local_context;

    const char *repo_basename = strrchr(entry->repo_name, '/');
    repo_basename = repo_basename != NULL ? repo_basename + 1 : entry->repo_name;

    char *output_path = NULL;
    EXIT_IF(asprintf(&output_path, "%s/CWE-%u_%s_%s_%.8s.json", config->export_folder_path, entry->cwe_id, entry->cve_id, repo_basename, entry->commit_hash) ==
                -1,
            "asprintf");

    char *prefix = commit_to_display(entry->cwe_id, entry->cve_id, entry->repo_name, entry->commit_hash);
    unsigned line_number = history_add_line(history, history->parsing_section, prefix, "parsing...");
    free(prefix);

    int fd = open(output_path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    free(output_path);

    if (fd == -1 && errno == EEXIST) {
        history_update_line(history, history->parsing_section, line_number, "file already exists", true);
        dataset_entry_destroy(entry);
        return;
    }

    history_update_line(history, history->parsing_section, line_number, "parsing...", false);

    EXIT_IF(fd == -1, "open");
    FILE *f = fdopen(fd, "w");
    EXIT_IF(f == NULL, "fdopen");

    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    EXIT_IF(doc == NULL, "yyjson_mut_doc_new");

    yyjson_mut_val *root = yyjson_mut_obj(doc);
    EXIT_IF(root == NULL, "yyjson_mut_obj");

    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_uint(doc, root, "CWE", entry->cwe_id);
    yyjson_mut_obj_add_str(doc, root, "CVE", entry->cve_id);
    yyjson_mut_obj_add_str(doc, root, "CVE_description", entry->cve_description);
    yyjson_mut_obj_add_str(doc, root, "repository", entry->repo_name);
    yyjson_mut_obj_add_str(doc, root, "commit_hash", entry->commit_hash);
    yyjson_mut_obj_add_str(doc, root, "commit_message", entry->commit_message);

    yyjson_mut_val *files = yyjson_mut_arr(doc);

    for (unsigned i = 0; i < entry->files_count; i++) {
        yyjson_mut_val *file = yyjson_mut_obj(doc);

        yyjson_mut_obj_add_str(doc, file, "current_commit_path", entry->files[i]->path);
        yyjson_mut_obj_add_str(doc, file, "previous_commit_path", entry->files[i]->previous_path);
        yyjson_mut_obj_add_str(doc, file, "status", entry->files[i]->status);
        yyjson_mut_obj_add_str(doc, file, "before", entry->files[i]->before);
        yyjson_mut_obj_add_str(doc, file, "after", entry->files[i]->after);

        yyjson_mut_arr_add_val(files, file);
    }

    yyjson_mut_obj_add_val(doc, root, "included_files", files);

    size_t len;
    const char *json = yyjson_mut_write(doc, YYJSON_WRITE_PRETTY, &len);
    EXIT_IF(json == NULL, "yyjson_mut_write");

    EXIT_IF(fwrite(json, 1, len, f) != len, "fwrite");

    free((void *)json);
    yyjson_mut_doc_free(doc);

    EXIT_IF(fclose(f) == EOF, "fclose");

    history_update_line(history, history->parsing_section, line_number, "complete", true);
    dataset_entry_destroy(entry);
}
