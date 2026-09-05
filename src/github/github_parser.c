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

bool github_parser_parse_infos(dataset_entry_t *entry, yyjson_doc *doc) {
    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *parents = yyjson_obj_get(root, "parents");
    EXIT_IF(parents == NULL || !yyjson_is_arr(parents), "parents");

    if (yyjson_arr_size(parents) != 1)
        return false;

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

    return true;
}

static dataset_file_t *parse_file(const config_t *config, yyjson_val *file) {
    yyjson_val *path = yyjson_obj_get(file, "filename");
    EXIT_IF(path == NULL || !yyjson_is_str(path), "filename");

    yyjson_val *status = yyjson_obj_get(file, "status");
    EXIT_IF(status == NULL || !yyjson_is_str(status), "status");

    const char *path_str = yyjson_get_str(path);
    const char *previous_path_str = path_str;
    const char *status_str = yyjson_get_str(status);

    EXIT_IF(strcmp(status_str, "modified") != 0 && strcmp(status_str, "added") != 0 && strcmp(status_str, "removed") != 0 && strcmp(status_str, "renamed") != 0,
            "file status \"%s\" not supported",
            status_str);

    if (!extension_is_supported(config, path_str))
        return NULL;

    if (strcmp(status_str, "renamed") == 0) {
        yyjson_val *previous_path = yyjson_obj_get(file, "previous_filename");
        EXIT_IF(previous_path == NULL || !yyjson_is_str(previous_path), "previous_filename");

        previous_path_str = yyjson_get_str(previous_path);

        if (!extension_is_supported(config, previous_path_str))
            return NULL;
    }

    else if (strcmp(status_str, "removed") == 0)
        path_str = NULL;

    else if (strcmp(status_str, "added") == 0)
        previous_path_str = NULL;

    return dataset_file_new(path_str, previous_path_str, status_str);
}

bool github_parser_parse_files(const config_t *config, dataset_entry_t *entry, yyjson_doc *doc) {
    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *files = yyjson_obj_get(root, "files");
    EXIT_IF(files == NULL || !yyjson_is_arr(files), "files");

    unsigned files_count = yyjson_arr_size(files);

    entry->files = calloc(files_count, sizeof(*entry->files));
    EXIT_IF(entry->files == NULL && files_count != 0, "calloc");

    entry->files_count = files_count;

    yyjson_val *file;
    size_t i, max;

    yyjson_arr_foreach(files, i, max, file) {
        dataset_file_t *dataset_file = parse_file(config, file);

        if (dataset_file == NULL)
            return false;

        entry->files[i] = dataset_file;
    }

    return true;
}
