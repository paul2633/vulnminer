#ifndef GITHUB_PARSER_H
#define GITHUB_PARSER_H

#include "config.h"
#include "dataset.h"
#include "yyjson.h"

unsigned github_parser_parse_commit(const config_t *config, dataset_entry_t *entry, yyjson_doc *doc);

#endif
