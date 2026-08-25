#include <curl/curl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <yyjson.h>

#include "dataset.h"
#include "github.h"
#include "github_parser.h"
#include "history.h"
#include "http.h"
#include "jobs.h"
#include "utils.h"

#define TIME_BETWEEN_REQUESTS 1

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

static long github_get_header_long(http_client_t *client, const char *name) {
    struct curl_header *header = NULL;

    CURLHcode err = curl_easy_header(client->curl, name, 0, CURLH_HEADER, -1, &header);

    if (err == CURLHE_MISSING || err == CURLHE_NOHEADERS)
        return -1;

    EXIT_IF(err != CURLHE_OK, "curl_easy_header");

    return strtol(header->value, NULL, 10);
}

static char *github_get_str(http_client_t *client, const char *url, size_t *response_size) {
    pthread_mutex_lock(&client->lock);
    sleep(TIME_BETWEEN_REQUESTS);

    while (true) {
        http_response_t response = {0};
        long status = 0;

        CURLcode err = http_get(client, url, &response, &status);

        if (err == CURLE_OPERATION_TIMEDOUT) {
            http_client_reset(client);
            free(response.data);
            continue;
        }

        EXIT_IF(err != CURLE_OK, curl_easy_strerror(err));

        if (status == 403) {
            long remaining = github_get_header_long(client, "x-ratelimit-remaining");
            long reset = github_get_header_long(client, "x-ratelimit-reset");

            EXIT_IF(remaining != 0, "HTTP error %ld with %ld remaining requests", status, remaining);
            EXIT_IF(reset < 0, "HTTP error %ld with missing x-ratelimit-reset", status);

            time_t now = time(NULL);
            EXIT_IF(reset < now, "HTTP error %ld with invalid x-ratelimit-reset", status);

            free(response.data);
            sleep((unsigned)(reset - now));
            continue;
        }

        if (status == 404 || status == 409 || status == 422) {
            pthread_mutex_unlock(&client->lock);
            free(response.data);
            return NULL;
        }

        if (status == 429) {
            free(response.data);
            sleep(60);
            continue;
        }

        if (status == 500 || status == 503) {
            free(response.data);
            sleep(10);
            continue;
        }

        EXIT_IF(status < 200 || status >= 300, "HTTP error %ld", status);

        pthread_mutex_unlock(&client->lock);

        *response_size = response.size;
        return response.data;
    }
}

static yyjson_doc *github_get_json(http_client_t *client, const char *url) {
    size_t response_size = 0;
    char *response_data = github_get_str(client, url, &response_size);

    if (response_data == NULL)
        return NULL;

    yyjson_doc *doc = yyjson_read(response_data, response_size, 0);
    free(response_data);
    EXIT_IF(doc == NULL, "yyjson_read");

    return doc;
}

static char *github_get_file(http_client_t *client, const char *repo_name, const char *path, const char *commit_hash, size_t *response_size) {
    CURLU *url = curl_url();
    EXIT_IF(url == NULL, "curl_url");

    CURLUcode err;

    err = curl_url_set(url, CURLUPART_SCHEME, "https", 0);
    EXIT_IF(err != CURLUE_OK, "curl_url_set");

    err = curl_url_set(url, CURLUPART_HOST, "github.com", 0);
    EXIT_IF(err != CURLUE_OK, "curl_url_set");

    char *url_path = NULL;
    EXIT_IF(asprintf(&url_path, "/%s/raw/%s/%s", repo_name, commit_hash, path) == -1, "asprintf");

    err = curl_url_set(url, CURLUPART_PATH, url_path, CURLU_URLENCODE);
    free(url_path);
    EXIT_IF(err != CURLUE_OK, "curl_url_set");

    char *url_str = NULL;
    err = curl_url_get(url, CURLUPART_URL, &url_str, 0);
    EXIT_IF(err != CURLUE_OK, "curl_url_get");

    char *content = github_get_str(client, url_str, response_size);

    curl_free(url_str);
    curl_url_cleanup(url);

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

static bool github_process_files(http_client_t *github_client, history_t *history, dataset_entry_t *entry, unsigned line_number) {
    for (unsigned i = 0; i < entry->files_count; i++) {
        dataset_file_t *file = entry->files[i];

        char *suffix = NULL;
        EXIT_IF(asprintf(&suffix, "fetching file %u/%u...", i + 1, entry->files_count) == -1, "asprintf");
        history_update_line(history, history->github_section, line_number, suffix, false);
        free(suffix);

        if (file->previous_path != NULL) {
            size_t before_size = 0;
            char *before = github_get_file(github_client, entry->repo_name, file->previous_path, entry->parent_commit_hash, &before_size);

            if (before == NULL)
                return false;

            file->before = before;
            file->before_size = before_size;
        }

        if (file->path != NULL) {
            size_t after_size = 0;
            char *after = github_get_file(github_client, entry->repo_name, file->path, entry->commit_hash, &after_size);

            if (after == NULL)
                return false;

            file->after = after;
            file->after_size = after_size;
        }
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

    char *prefix = commit_to_display(entry->cwe_id, entry->cve_id, entry->repo_name, entry->commit_hash);
    unsigned line_number = history_add_line(history, history->github_section, prefix, "probing...");
    free(prefix);

    char *url = NULL;
    EXIT_IF(asprintf(&url, "https://api.github.com/repos/%s/commits/%s", entry->repo_name, entry->commit_hash) == -1, "asprintf");

    yyjson_doc *doc = github_get_json(github_client, url);
    free(url);

    if (doc == NULL) {
        history_update_line(history, history->github_section, line_number, "commit not found", true);
        dataset_entry_destroy(entry);
        return;
    }

    if (!github_parser_parse_infos(entry, doc)) {
        history_update_line(history, history->github_section, line_number, "parent commit not found", true);
        yyjson_doc_free(doc);
        dataset_entry_destroy(entry);
        return;
    }

    if (!github_parser_parse_files(config, entry, doc)) {
        history_update_line(history, history->github_section, line_number, "unsupported extension found", true);
        yyjson_doc_free(doc);
        dataset_entry_destroy(entry);
        return;
    }

    yyjson_doc_free(doc);

    if (!github_process_files(github_client, history, entry, line_number)) {
        history_update_line(history, history->github_section, line_number, "file content unavailable", true);
        dataset_entry_destroy(entry);
        return;
    }

    history_update_line(history, history->github_section, line_number, "complete", true);

    history_increment_pending(history, history->parsing_section);
    push_new_job(parsing_queue, entry);
}
