#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <yyjson.h>

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

static void nvd_parser_parse_cve(yyjson_val *cve) {
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
        if (strstr(url_str, "github.com") == NULL || strstr(url_str, "/commit/") == NULL)
            continue;

        yyjson_val *published = yyjson_obj_get(cve, "published");
        if (published == NULL || !yyjson_is_str(published))
            continue;

        yyjson_val *id = yyjson_obj_get(cve, "id");
        EXIT_IF(id == NULL || !yyjson_is_str(id), "id");

        // printf("%s %s %s\n", yyjson_get_str(id), yyjson_get_str(published), url_str);
    }
}

void nvd_parser_extract_commits(yyjson_doc *doc) {
    yyjson_val *root = yyjson_doc_get_root(doc);
    EXIT_IF(root == NULL, "yyjson_doc_get_root");

    yyjson_val *vulns = yyjson_obj_get(root, "vulnerabilities");
    EXIT_IF(vulns == NULL || !yyjson_is_arr(vulns), "vulnerabilities");

    size_t count = yyjson_arr_size(vulns);

    for (size_t i = count; i-- > 0;) {
        yyjson_val *vuln = yyjson_arr_get(vulns, i);

        yyjson_val *cve = yyjson_obj_get(vuln, "cve");
        EXIT_IF(cve == NULL || !yyjson_is_obj(cve), "cve");

        nvd_parser_parse_cve(cve);
    }
}
