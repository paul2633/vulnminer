#ifndef NVD_PARSER_H
#define NVD_PARSER_H

#include <yyjson.h>

#include "history.h"
#include "jobs.h"

int nvd_parser_get_total_results(yyjson_doc *doc);

void nvd_parser_extract_commits(yyjson_doc *doc, jobs_queue_t *github_queue, unsigned cwe_id, history_t *history);

#endif
