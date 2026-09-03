#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <yyjson.h>

#include "dataset.h"
#include "github.h"
#include "github_parser.h"
#include "history.h"
#include "http.h"
#include "jobs.h"
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

static char *github_get_file(http_client_t *client, const char *repo_name, const char *path, const char *commit_hash, size_t *response_size) {
    char *url = NULL;
    EXIT_IF(asprintf(&url, "https://github.com/%s/raw/%s/%s", repo_name, commit_hash, path) == -1, "asprintf");

    char *content = http_get_str(client, url, response_size);
    free(url);

    return content;
}

char *commit_to_display(unsigned cwe_id, const char *cve_id, const char *repo_name, const char *commit_hash) {
    char *commit_str = NULL;
    EXIT_IF(asprintf(&commit_str, "%s@%s", repo_name, commit_hash) == -1, "asprintf");
    char *prefix = NULL;
    EXIT_IF(asprintf(&prefix, "CWE-%-3u   %-15s   %-32.32s", cwe_id, cve_id, commit_str) == -1, "asprintf");
    free(commit_str);
    return prefix;
}

static unsigned github_process_files(http_client_t *github_client, history_t *history, dataset_entry_t *entry, unsigned line_number,
                                     unsigned files_to_download) {
    unsigned downloaded_files = 0, current_file = 0;

    for (size_t i = 0; i < entry->files_count; i++) {
        dataset_file_t *file = entry->files[i];
        if (file->state != ONGOING)
            continue;

        char *suffix = NULL;
        EXIT_IF(asprintf(&suffix, "fetching file %u/%u...", ++current_file, files_to_download) == -1, "asprintf");
        history_update_line(history, history->github_section, line_number, suffix, false);
        free(suffix);

        size_t before_size = 0;
        char *before = github_get_file(github_client, entry->repo_name, file->path, entry->parent_commit_hash, &before_size);

        if (before == NULL) {
            file->state = FILE_CONTENT_UNAVAILABLE;
            continue;
        }

        size_t after_size = 0;
        char *after = github_get_file(github_client, entry->repo_name, file->path, entry->commit_hash, &after_size);

        if (after == NULL) {
            free(before);
            file->state = FILE_CONTENT_UNAVAILABLE;
            continue;
        }

        file->before = before;
        file->before_size = before_size;
        file->after = after;
        file->after_size = after_size;

        downloaded_files++;
    }

    return downloaded_files;
}

void github_process_commit(void *global_context, void *local_context) {

    github_global_context_t *global = global_context;
    const config_t *config = global->config;
    http_client_t *github_client = global->github_client;
    history_t *history = global->history;
    jobs_queue_t *parsing_queue = global->parsing_queue;

    dataset_entry_t *entry = local_context;

    char *prefix = commit_to_display(entry->cwe_id, entry->cve_id, entry->repo_name, entry->commit_hash);
    unsigned line_number = history_add_line(history, history->github_section, prefix, "probing...");
    free(prefix);

    char *url = NULL;
    EXIT_IF(asprintf(&url, "https://api.github.com/repos/%s/commits/%s", entry->repo_name, entry->commit_hash) == -1, "asprintf");

    yyjson_doc *doc = http_get_json(github_client, url);
    free(url);

    if (doc == NULL) {
        history_update_line(history, history->github_section, line_number, "page not found", true);
        dataset_entry_destroy(entry);
        return;
    }

    unsigned files_to_download = github_parser_parse_commit(config, entry, doc);
    yyjson_doc_free(doc);

    if (entry->parent_commit_hash == NULL) {
        history_update_line(history, history->github_section, line_number, "parent commit not found", true);
        dataset_entry_destroy(entry);
        return;
    }

    if (github_process_files(github_client, history, entry, line_number, files_to_download) == 0) {
        history_update_line(history, history->github_section, line_number, "no valid file found", true);
        dataset_entry_destroy(entry);
        return;
    }

    history_update_line(history, history->github_section, line_number, "complete", true);

    history_increment_pending(history, history->parsing_section);
    push_new_job(parsing_queue, entry);
}
