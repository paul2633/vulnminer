#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yyjson.h>

#include "dataset.h"
#include "history.h"
#include "jobs.h"
#include "nvd_parser.h"
#include "utils.h"

#define COMMIT_HASH_LEN 40

#define REPO_NAME_INDEX 0
#define COMMIT_HASH_INDEX 1

int nvd_parser_get_total_results(yyjson_doc *doc) {
    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *total = yyjson_obj_get(root, "totalResults");
    EXIT_IF(total == NULL || !yyjson_is_uint(total), "totalResults");

    return yyjson_get_int(total);
}

static char *nvd_parser_get_description(yyjson_val *cve) {
    yyjson_val *descriptions = yyjson_obj_get(cve, "descriptions");
    if (descriptions == NULL || !yyjson_is_arr(descriptions))
        return NULL;

    yyjson_val *description;
    size_t i, max;

    yyjson_arr_foreach(descriptions, i, max, description) {
        yyjson_val *lang = yyjson_obj_get(description, "lang");
        yyjson_val *value = yyjson_obj_get(description, "value");

        if (lang == NULL || !yyjson_is_str(lang) || value == NULL || !yyjson_is_str(value))
            continue;

        if (strcmp(yyjson_get_str(lang), "en") == 0)
            return strdup(yyjson_get_str(value));
    }

    return NULL;
}

static bool nvd_parser_check_is_patch(yyjson_val *tags) {
    if (tags == NULL || !yyjson_is_arr(tags))
        return false;

    yyjson_val *tag;
    size_t i, max;

    yyjson_arr_foreach(tags, i, max, tag) {
        if (yyjson_is_str(tag) && strcmp(yyjson_get_str(tag), "Patch") == 0)
            return true;
    }

    return false;
}

static char *extract_repo_path(const char *url) {
    const char *prefix = "https://github.com/";
    const char *suffix = "/commit/";

    if (strncmp(url, prefix, strlen(prefix)) != 0)
        return NULL;

    const char *p = url + strlen(prefix);
    const char *commit = strstr(p, suffix);

    if (commit == NULL)
        return NULL;

    return strndup(p, commit - p);
}

static char *extract_commit_hash(const char *url) {
    const char *prefix = "https://github.com/";
    const char *suffix = "/commit/";

    if (strncmp(url, prefix, strlen(prefix)) != 0)
        return NULL;

    const char *commit = strstr(url + strlen(prefix), suffix);

    if (commit == NULL)
        return NULL;

    char *commit_hash = strdup(commit + strlen(suffix));

    if (commit_hash == NULL)
        return NULL;

    if (strlen(commit_hash) > COMMIT_HASH_LEN)
        commit_hash[COMMIT_HASH_LEN] = '\0';

    return commit_hash;
}

static bool check_is_duplicate(char *pushed_repos_infos[][2], size_t pushed_repos_count, const char *repo_name, const char *commit_hash) {
    for (size_t i = 0; i < pushed_repos_count; i++) {
        if (strcmp(repo_name, pushed_repos_infos[i][REPO_NAME_INDEX]) == 0 && strcmp(commit_hash, pushed_repos_infos[i][COMMIT_HASH_INDEX]) == 0) {
            return true;
        }
    }

    return false;
}

static void nvd_parser_parse_cve(jobs_queue_t *github_queue, yyjson_val *cve, unsigned cwe_id, history_t *history) {
    yyjson_val *refs = yyjson_obj_get(cve, "references");
    if (refs == NULL || !yyjson_is_arr(refs))
        return;

    char *cve_description = nvd_parser_get_description(cve);
    size_t refs_count = yyjson_arr_size(refs);

    if (refs_count == 0) {
        free(cve_description);
        return;
    }

    char *pushed_repos_infos[refs_count][2];
    size_t pushed_repos_count = 0;

    yyjson_val *ref;
    size_t i, max;

    yyjson_arr_foreach(refs, i, max, ref) {
        yyjson_val *url = yyjson_obj_get(ref, "url");
        if (url == NULL || !yyjson_is_str(url))
            continue;

        if (!nvd_parser_check_is_patch(yyjson_obj_get(ref, "tags")))
            continue;

        const char *url_str = yyjson_get_str(url);

        char *repo_name = extract_repo_path(url_str);
        char *commit_hash = extract_commit_hash(url_str);

        if (repo_name == NULL || commit_hash == NULL || check_is_duplicate(pushed_repos_infos, pushed_repos_count, repo_name, commit_hash)) {
            free(repo_name);
            free(commit_hash);
            continue;
        }

        pushed_repos_infos[pushed_repos_count][REPO_NAME_INDEX] = repo_name;
        pushed_repos_infos[pushed_repos_count][COMMIT_HASH_INDEX] = commit_hash;
        pushed_repos_count++;

        const char *cve_id = yyjson_get_str(yyjson_obj_get(cve, "id"));
        EXIT_IF(cve_id == NULL, "id");

        dataset_entry_t *entry = dataset_entry_new(cwe_id, cve_id, repo_name, commit_hash, cve_description);

        history_increment_pending(history, history->github_section);
        push_new_job(github_queue, entry);
    }

    for (i = 0; i < pushed_repos_count; i++) {
        free(pushed_repos_infos[i][REPO_NAME_INDEX]);
        free(pushed_repos_infos[i][COMMIT_HASH_INDEX]);
    }

    free(cve_description);
}

void nvd_parser_extract_commits(yyjson_doc *doc, jobs_queue_t *github_queue, unsigned cwe_id, history_t *history) {
    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *vulns = yyjson_obj_get(root, "vulnerabilities");
    EXIT_IF(vulns == NULL || !yyjson_is_arr(vulns), "vulnerabilities");

    size_t count = yyjson_arr_size(vulns);

    for (size_t i = count; i-- > 0;) {
        yyjson_val *vuln = yyjson_arr_get(vulns, i);

        yyjson_val *cve = yyjson_obj_get(vuln, "cve");
        EXIT_IF(cve == NULL || !yyjson_is_obj(cve), "cve");

        nvd_parser_parse_cve(github_queue, cve, cwe_id, history);
    }
}
