#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yyjson.h>

#include "github.h"
#include "history.h"
#include "http.h"
#include "utils.h"

http_client_t *github_client_new(const char *api_key) {
    http_client_t *github_client = http_client_new();

    if (api_key != NULL) {
        char *line = NULL;

        EXIT_IF(asprintf(&line, "Authorization: Bearer %s", api_key) == -1, "asprintf");

        http_client_add_header(github_client, line);
        free(line);
    }

    http_client_add_header(github_client, "Accept: application/vnd.github+json");
    http_client_add_header(github_client, "User-Agent: repo-analyzer");

    return github_client;
}

static void parse_commit_info(github_commit_t *commit, yyjson_val *root) {
    yyjson_val *sha = yyjson_obj_get(root, "sha");
    EXIT_IF(sha == NULL || !yyjson_is_str(sha), "sha");

    commit->current_commit_hash = strdup(yyjson_get_str(sha));
    EXIT_IF(commit->current_commit_hash == NULL, "strdup");

    yyjson_val *parents = yyjson_obj_get(root, "parents");
    EXIT_IF(parents == NULL || !yyjson_is_arr(parents), "parents");

    if (yyjson_arr_size(parents) != 1)
        return;

    yyjson_val *parent = yyjson_arr_get_first(parents);
    yyjson_val *parent_sha = yyjson_obj_get(parent, "sha");

    EXIT_IF(parent_sha == NULL || !yyjson_is_str(parent_sha), "parent sha");

    commit->parent_commit_hash = strdup(yyjson_get_str(parent_sha));
    EXIT_IF(commit->parent_commit_hash == NULL, "strdup");
}

static void parse_commit_files(github_commit_t *commit, yyjson_val *root) {
    yyjson_val *files = yyjson_obj_get(root, "files");
    EXIT_IF(files == NULL || !yyjson_is_arr(files), "files");

    commit->files_count = yyjson_arr_size(files);

    if (commit->files_count == 0)
        return;

    commit->files = calloc(commit->files_count, sizeof(*commit->files));
    EXIT_IF(commit->files == NULL, "calloc");

    yyjson_val *file;
    size_t i, max;

    yyjson_arr_foreach(files, i, max, file) {
        yyjson_val *path = yyjson_obj_get(file, "filename");
        yyjson_val *status = yyjson_obj_get(file, "status");

        EXIT_IF(path == NULL || !yyjson_is_str(path), "filename");
        EXIT_IF(status == NULL || !yyjson_is_str(status), "status");

        commit->files[i].path = strdup(yyjson_get_str(path));
        EXIT_IF(commit->files[i].path == NULL, "strdup");

        commit->files[i].status = strdup(yyjson_get_str(status));
        EXIT_IF(commit->files[i].status == NULL, "strdup");
    }
}

static github_commit_t *parse_commit_json(yyjson_doc *doc) {
    github_commit_t *commit = calloc(1, sizeof(*commit));
    EXIT_IF(commit == NULL, "calloc");

    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    parse_commit_info(commit, root);
    parse_commit_files(commit, root);

    return commit;
}

static void github_commit_destroy(github_commit_t *commit) {
    free(commit->current_commit_hash);
    free(commit->parent_commit_hash);

    for (unsigned i = 0; i < commit->files_count; i++) {
        free(commit->files[i].path);
        free(commit->files[i].status);
    }

    free(commit->files);
    free(commit);
}

void github_parse_commit(http_client_t *client, history_t *history, unsigned cwe_id, const char *cve_id, const char *repo_name, const char *commit_hash) {
    char *url = NULL;
    EXIT_IF(asprintf(&url, "https://api.github.com/repos/%s/commits/%s", repo_name, commit_hash) == -1, "asprintf");

    char *commit_str = NULL;
    EXIT_IF(asprintf(&commit_str, "%s@%s", repo_name, commit_hash) == -1, "asprintf");
    char *line = NULL;
    EXIT_IF(asprintf(&line, "%-15s   %-32.32s...   probing...", cve_id, commit_str) == -1, "asprintf");
    free(commit_str);

    unsigned line_number = history_push(history, history->github_section, line, true);

    yyjson_doc *doc = http_get_json(client, url);

    if (doc == NULL)
        history_append(history, history->github_section, line_number, "   page not found", HISTORY_STATUS_FAILED);
    else
        history_append(history, history->github_section, line_number, "   complete", HISTORY_STATUS_SUCCEEDED);

    free(url);

    if (doc == NULL)
        return;

    github_commit_t *commit = parse_commit_json(doc);
    yyjson_doc_free(doc);

    if (commit->files_count == 0 || commit->parent_commit_hash == NULL) {
        github_commit_destroy(commit);
        return;
    }

    github_commit_destroy(commit);
    // TODO
}
