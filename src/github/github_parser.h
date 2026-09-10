#ifndef GITHUB_PARSER_H
#define GITHUB_PARSER_H

#include <stdbool.h>

#include "config.h"
#include "dataset.h"
#include "yyjson.h"

void github_parse_commit_infos(dataset_entry_t *entry, yyjson_doc *doc);

bool github_parse_commit_files(const config_t *config, dataset_entry_t *entry, yyjson_doc *doc);

void github_parse_context_files(dataset_entry_t *entry, yyjson_doc *doc, unsigned context_depth);

#endif
