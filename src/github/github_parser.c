#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <yyjson.h>

#include "dataset.h"
#include "github_parser.h"
#include "utils.h"

static bool extension_is_supported(const config_t *config, const char *path) {
    const char *ext = strrchr(path, '.');

    if (ext == NULL)
        return false;

    if (config->include_c_files && (strcmp(ext, ".c") == 0 || strcmp(ext, ".h") == 0))
        return true;

    if (config->include_cpp_files && (strcmp(ext, ".cpp") == 0 || strcmp(ext, ".hpp") == 0))
        return true;

    return false;
}

static unsigned parse_commit_files(const config_t *config, dataset_entry_t *entry, yyjson_val *root) {
    yyjson_val *files = yyjson_obj_get(root, "files");
    EXIT_IF(files == NULL || !yyjson_is_arr(files), "files");

    unsigned files_count = yyjson_arr_size(files);

    entry->files = calloc(files_count, sizeof(*entry->files));
    EXIT_IF(entry->files == NULL && files_count != 0, "calloc");

    entry->files_count = files_count;

    yyjson_val *file;
    size_t i, max;

    unsigned files_to_download = 0;

    yyjson_arr_foreach(files, i, max, file) {
        yyjson_val *path = yyjson_obj_get(file, "filename");
        EXIT_IF(path == NULL || !yyjson_is_str(path), "filename");

        entry->files[i] = dataset_file_new(yyjson_get_str(path));

        yyjson_val *status = yyjson_obj_get(file, "status");
        EXIT_IF(status == NULL || !yyjson_is_str(status), "status");

        if (strcmp(yyjson_get_str(status), "modified") != 0) {
            entry->files[i]->state = FILE_NOT_MODIFIED;
            continue;
        }

        if (!extension_is_supported(config, entry->files[i]->path)) {
            entry->files[i]->state = EXTENSION_NOT_SUPPORTED;
            continue;
        }

        files_to_download++;
    }

    return files_to_download;
}

unsigned github_parser_parse_commit(const config_t *config, dataset_entry_t *entry, yyjson_doc *doc) {
    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *parents = yyjson_obj_get(root, "parents");
    EXIT_IF(parents == NULL || !yyjson_is_arr(parents), "parents");

    if (yyjson_arr_size(parents) != 1)
        return 0;

    yyjson_val *parent = yyjson_arr_get_first(parents);
    yyjson_val *parent_sha = yyjson_obj_get(parent, "sha");

    EXIT_IF(parent_sha == NULL || !yyjson_is_str(parent_sha), "parent sha");

    entry->parent_commit_hash = strdup(yyjson_get_str(parent_sha));
    EXIT_IF(entry->parent_commit_hash == NULL, "strdup");

    yyjson_val *commit = yyjson_obj_get(root, "commit");
    EXIT_IF(commit == NULL || !yyjson_is_obj(commit), "commit");

    yyjson_val *message = yyjson_obj_get(commit, "message");
    EXIT_IF(message == NULL || !yyjson_is_str(message), "message");

    entry->commit_message = strdup(yyjson_get_str(message));
    EXIT_IF(entry->commit_message == NULL, "strdup");

    return parse_commit_files(config, entry, root);
}
