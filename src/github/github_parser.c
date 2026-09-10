#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <yyjson.h>

#include "dataset.h"
#include "github_parser.h"
#include "utils.h"

void github_parse_commit_infos(dataset_entry_t *entry, yyjson_doc *doc) {
    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *parents = yyjson_obj_get(root, "parents");
    EXIT_IF(parents == NULL || !yyjson_is_arr(parents), "parents");

    if (yyjson_arr_size(parents) != 1)
        return;

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
}

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

bool github_parse_commit_files(const config_t *config, dataset_entry_t *entry, yyjson_doc *doc) {
    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *files = yyjson_obj_get(root, "files");
    EXIT_IF(files == NULL || !yyjson_is_arr(files), "files");

    yyjson_val *file;
    size_t i, max;

    yyjson_arr_foreach(files, i, max, file) {
        const char *status = yyjson_get_str(yyjson_obj_get(file, "status"));
        EXIT_IF(status == NULL, "status");

        EXIT_IF(strcmp(status, "modified") != 0 && strcmp(status, "added") != 0 && strcmp(status, "removed") != 0 && strcmp(status, "renamed") != 0,
                "file status \"%s\" not supported",
                status);

        const char *path = yyjson_get_str(yyjson_obj_get(file, "filename"));
        EXIT_IF(path == NULL, "filename");
        const char *previous_path = path;

        if (!extension_is_supported(config, path))
            return false;

        if (strcmp(status, "renamed") == 0) {
            previous_path = yyjson_get_str(yyjson_obj_get(file, "previous_filename"));
            EXIT_IF(previous_path == NULL, "previous_filename");

            const char *old_ext = strrchr(previous_path, '.');
            const char *new_ext = strrchr(path, '.');

            if (old_ext == NULL || new_ext == NULL || strcmp(old_ext, new_ext) != 0)
                return false;
        }

        if (strcmp(status, "removed") == 0)
            path = NULL;

        if (strcmp(status, "added") == 0)
            previous_path = NULL;

        add_new_file(entry, path, previous_path, status);
    }
    return true;
}

static unsigned context_distance(const char *path1, const char *path2) {
    unsigned common_len = 0, distance = 0;

    while (path1[common_len] == path2[common_len] && path1[common_len] != '\0')
        common_len++;

    while (common_len > 0 && path1[common_len - 1] != '/')
        common_len--;

    for (unsigned i = common_len; path1[i] != '\0'; i++)
        if (path1[i] == '/')
            distance++;

    for (unsigned i = common_len; path2[i] != '\0'; i++)
        if (path2[i] == '/')
            distance++;

    return distance;
}

static bool is_already_in_commit(const dataset_entry_t *entry, const char *path) {
    for (unsigned i = 0; i < entry->files_count; i++)
        if (entry->files[i]->path != NULL && strcmp(path, entry->files[i]->path) == 0)
            return true;

    return false;
}

static void github_add_context_file(dataset_entry_t *entry, const char *path, unsigned context_depth) {
    dataset_context_file_t *context_file = NULL;

    for (unsigned i = 0; i < entry->files_count; i++) {
        if (entry->files[i]->path == NULL)
            continue;

        unsigned distance = context_distance(entry->files[i]->path, path);

        if (distance < context_depth) {
            if (context_file == NULL)
                context_file = add_and_get_new_context_file(entry, path);
            add_new_context_distance(context_file, entry->files[i]->path, distance);
        }
    }
}

void github_parse_context_files(dataset_entry_t *entry, yyjson_doc *doc, unsigned context_depth) {
    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *tree = yyjson_obj_get(root, "tree");
    EXIT_IF(tree == NULL || !yyjson_is_arr(tree), "tree");

    yyjson_val *file;
    size_t i, max;

    yyjson_arr_foreach(tree, i, max, file) {
        const char *path = yyjson_get_str(yyjson_obj_get(file, "path"));
        EXIT_IF(path == NULL, "path");

        const char *type = yyjson_get_str(yyjson_obj_get(file, "type"));
        EXIT_IF(type == NULL, "type");

        if (strcmp(type, "blob") != 0)
            continue;

        if (is_already_in_commit(entry, path))
            continue;

        github_add_context_file(entry, path, context_depth);
    }
}
