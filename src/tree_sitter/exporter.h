#ifndef EXPORTER_H
#define EXPORTER_H

#include <stdbool.h>

#include "dataset.h"

bool exporter_export_commit(const char *export_folder_path, dataset_entry_t *entry);

#endif
