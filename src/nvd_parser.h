#ifndef NVD_PARSER_H
#define NVD_PARSER_H

#include <yyjson.h>

#include "history.h"
#include "http.h"

int nvd_parser_get_total_results(yyjson_doc *doc);

void nvd_parser_extract_commits(yyjson_doc *doc, http_client_t *github_client, history_t *history);

#endif
