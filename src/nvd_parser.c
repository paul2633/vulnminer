#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yyjson.h>

#include "jobs.h"
#include "nvd_parser.h"
#include "utils.h"

int nvd_parser_get_total_results(yyjson_doc *doc) {
    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *total = yyjson_obj_get(root, "totalResults");
    EXIT_IF(total == NULL || !yyjson_is_uint(total), "totalResults");

    return yyjson_get_int(total);
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

    return strdup(commit + strlen(suffix));
}

static void nvd_parser_parse_cve(jobs_queue_t *queue, yyjson_val *cve, unsigned cwe_id) {
    yyjson_val *refs = yyjson_obj_get(cve, "references");
    if (refs == NULL || !yyjson_is_arr(refs))
        return;

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

        if (repo_name == NULL || commit_hash == NULL) {
            free(repo_name);
            free(commit_hash);
            continue;
        }

        const char *id = yyjson_get_str(yyjson_obj_get(cve, "id"));
        EXIT_IF(id == NULL, "id");

        char *id_cpy = strdup(id);
        EXIT_IF(id_cpy == NULL, "strdup");

        push_new_job(queue, cwe_id, id_cpy, repo_name, commit_hash);
    }
}

void nvd_parser_extract_commits(yyjson_doc *doc, jobs_queue_t *queue, unsigned cwe_id) {
    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *vulns = yyjson_obj_get(root, "vulnerabilities");
    EXIT_IF(vulns == NULL || !yyjson_is_arr(vulns), "vulnerabilities");

    size_t count = yyjson_arr_size(vulns);

    for (size_t i = count; i-- > 0;) {
        yyjson_val *vuln = yyjson_arr_get(vulns, i);

        yyjson_val *cve = yyjson_obj_get(vuln, "cve");
        EXIT_IF(cve == NULL || !yyjson_is_obj(cve), "cve");

        nvd_parser_parse_cve(queue, cve, cwe_id);
    }
}
