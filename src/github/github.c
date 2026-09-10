#include <curl/curl.h>
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

    char *line = NULL;
    EXIT_IF(asprintf(&line, "Authorization: Bearer %s", api_key) == -1, "asprintf");

    http_client_add_header(github_client, line);
    free(line);

    http_client_add_header(github_client, "Accept: application/vnd.github+json");
    http_client_add_header(github_client, "User-Agent: vulnminer");

    return github_client;
}

static yyjson_doc *github_download_json(http_client_t *client, const char *url) {
    size_t response_size = 0;
    char *response_data = github_download(client, url, &response_size);

    if (response_data == NULL)
        return NULL;

    yyjson_doc *doc = yyjson_read(response_data, response_size, 0);
    free(response_data);
    EXIT_IF(doc == NULL, "yyjson_read");

    return doc;
}

static char *github_download_file(http_client_t *client, const char *repo_name, const char *path, const char *commit_hash, size_t *response_size) {
    CURLU *url = curl_url();
    EXIT_IF(url == NULL, "curl_url");

    EXIT_IF(curl_url_set(url, CURLUPART_SCHEME, "https", 0) != CURLUE_OK, "curl_url_set");
    EXIT_IF(curl_url_set(url, CURLUPART_HOST, "github.com", 0) != CURLUE_OK, "curl_url_set");

    char *url_path = NULL;
    EXIT_IF(asprintf(&url_path, "/%s/raw/%s/%s", repo_name, commit_hash, path) == -1, "asprintf");

    EXIT_IF(curl_url_set(url, CURLUPART_PATH, url_path, CURLU_URLENCODE) != CURLUE_OK, "curl_url_set");
    free(url_path);

    char *url_str = NULL;
    EXIT_IF(curl_url_get(url, CURLUPART_URL, &url_str, 0) != CURLUE_OK, "curl_url_get");

    char *content = github_download(client, url_str, response_size);

    curl_free(url_str);
    curl_url_cleanup(url);
    return content;
}

char *commit_to_display(unsigned cwe_id, const char *cve_id, const char *cve_published, const char *repo_name, const char *commit_hash) {
    struct tm tm = {0};
    EXIT_IF(strptime(cve_published, "%Y-%m-%dT%H:%M:%S", &tm) == NULL, "strptime");

    char published_display[sizeof("DD/MM/YYYY")];
    EXIT_IF(strftime(published_display, sizeof(published_display), "%d/%m/%Y", &tm) == 0, "strftime");

    char *commit_str = NULL;
    EXIT_IF(asprintf(&commit_str, "%s@%s", repo_name, commit_hash) == -1, "asprintf");

    char *prefix = NULL;
    EXIT_IF(asprintf(&prefix, "CWE-%-3u   %-15s   %s   %-32.32s", cwe_id, cve_id, published_display, commit_str) == -1, "asprintf");

    free(commit_str);
    return prefix;
}

static bool github_process_files(http_client_t *github_client, history_t *history, dataset_entry_t *entry, unsigned line_number) {
    for (unsigned i = 0; i < entry->files_count; i++) {
        dataset_file_t *file = entry->files[i];

        char *suffix = NULL;
        EXIT_IF(asprintf(&suffix, "downloading content of commit file %u/%u...", i + 1, entry->files_count) == -1, "asprintf");
        history_update_line(history, history->github_section, line_number, suffix, false);
        free(suffix);

        if (file->previous_path != NULL) {
            file->before = github_download_file(github_client, entry->repo_name, file->previous_path, entry->parent_commit_hash, &file->before_size);
            if (file->before == NULL)
                return false;
        }

        if (file->path != NULL) {
            file->after = github_download_file(github_client, entry->repo_name, file->path, entry->commit_hash, &file->after_size);
            if (file->after == NULL)
                return false;
        }
    }

    for (unsigned i = 0; i < entry->context_files_count; i++) {
        dataset_context_file_t *context_file = entry->context_files[i];

        char *suffix = NULL;
        EXIT_IF(asprintf(&suffix, "downloading content of context file %u/%u...", i + 1, entry->context_files_count) == -1, "asprintf");
        history_update_line(history, history->github_section, line_number, suffix, false);
        free(suffix);

        context_file->content = github_download_file(github_client, entry->repo_name, context_file->path, entry->commit_hash, &context_file->size);
        if (context_file->content == NULL)
            return false;
    }

    return true;
}

void github_process_commit(void *global_context, void *local_context) {
    github_global_context_t *global = global_context;

    const config_t *config = global->config;
    http_client_t *github_client = global->github_client;
    history_t *history = global->history;
    jobs_queue_t *parsing_queue = global->parsing_queue;
    dataset_entry_t *entry = local_context;

    char *prefix = commit_to_display(entry->cwe_id, entry->cve_id, entry->cve_published, entry->repo_name, entry->commit_hash);
    unsigned line_number = history_add_line(history, history->github_section, prefix, "probing commit...");
    free(prefix);

    char *url = NULL;
    EXIT_IF(asprintf(&url, "https://api.github.com/repos/%s/commits/%s", entry->repo_name, entry->commit_hash) == -1, "asprintf");

    yyjson_doc *doc = github_download_json(github_client, url);
    free(url);

    if (doc == NULL) {
        history_update_line(history, history->github_section, line_number, "commit rejected: commit unavailable", true);
        dataset_entry_destroy(entry);
        return;
    }

    github_parse_commit_infos(entry, doc);

    if (entry->parent_commit_hash == NULL) {
        history_update_line(history, history->github_section, line_number, "commit rejected: more than one parent", true);
        yyjson_doc_free(doc);
        dataset_entry_destroy(entry);
        return;
    }

    if (!github_parse_commit_files(config, entry, doc)) {
        history_update_line(history, history->github_section, line_number, "commit rejected: file with excluded extension found", true);
        yyjson_doc_free(doc);
        dataset_entry_destroy(entry);
        return;
    }

    yyjson_doc_free(doc);

    if (entry->files_count == 0) {
        history_update_line(history, history->github_section, line_number, "commit rejected: commit is empty", true);
        dataset_entry_destroy(entry);
        return;
    }

    if (entry->files_count > config->max_files_commit) {
        history_update_line(history, history->github_section, line_number, "commit rejected: too many files affected by commit", true);
        dataset_entry_destroy(entry);
        return;
    }

    if (config->context_depth > 0) {
        history_update_line(history, history->github_section, line_number, "probing context...", false);

        url = NULL;
        EXIT_IF(asprintf(&url, "https://api.github.com/repos/%s/git/trees/%s?recursive=1", entry->repo_name, entry->commit_hash) == -1, "asprintf");
        doc = github_download_json(github_client, url);
        free(url);

        if (doc == NULL) {
            history_update_line(history, history->github_section, line_number, "commit rejected: context unavailable", true);
            dataset_entry_destroy(entry);
            return;
        }

        github_parse_context_files(entry, doc, config->context_depth);
        yyjson_doc_free(doc);
    }

    if (entry->context_files_count > config->max_files_context) {
        history_update_line(history, history->github_section, line_number, "commit rejected: too many files in context", true);
        dataset_entry_destroy(entry);
        return;
    }

    if (entry->files_count + entry->context_files_count > config->max_files_total) {
        history_update_line(history, history->github_section, line_number, "commit rejected: too many files in total", true);
        dataset_entry_destroy(entry);
        return;
    }

    if (!github_process_files(github_client, history, entry, line_number)) {
        history_update_line(history, history->github_section, line_number, "commit rejected: file content unavailable", true);
        dataset_entry_destroy(entry);
        return;
    }

    history_update_line(history, history->github_section, line_number, "commit accepted", true);
    history_increment_pending(history, history->parsing_section);
    // push_new_job(parsing_queue, entry);
    dataset_entry_destroy(entry);
}
