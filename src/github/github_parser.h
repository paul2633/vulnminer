#ifndef GITHUB_PARSER_H
#define GITHUB_PARSER_H

#include <stdbool.h>

#include "config.h"
#include "dataset.h"
#include "yyjson.h"

bool github_parser_parse_infos(dataset_entry_t *entry, yyjson_doc *doc);

bool github_parser_parse_files(const config_t *config, dataset_entry_t *entry, yyjson_doc *doc);

#endif
