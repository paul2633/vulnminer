#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dataset.h"
#include "github/github.h"
#include "parser.h"
#include "utils.h"

static void json_write(FILE *f, unsigned indent, const char *fmt, ...) {
    for (unsigned i = 0; i < indent; i++)
        EXIT_IF(fputs("    ", f) == EOF, "fputs");

    va_list ap;
    va_start(ap, fmt);
    EXIT_IF(vfprintf(f, fmt, ap) < 0, "vfprintf");
    va_end(ap);
}

void parser_export_commit(void *global_context, void *local_context) {

    parser_global_context_t *global = global_context;
    config_t *config = global->config;
    history_t *history = global->history;

    dataset_entry_t *entry = local_context;

    const char *repo_basename = strrchr(entry->repo_name, '/');
    repo_basename = repo_basename != NULL ? repo_basename + 1 : entry->repo_name;

    char *output_path = NULL;
    EXIT_IF(asprintf(&output_path, "%s/CWE-%u_%s_%s_%.8s.json", config->export_folder_path, entry->cwe_id, entry->cve_id, repo_basename, entry->commit_hash) ==
                -1,
            "asprintf");

    char *prefix = commit_to_display(entry->cwe_id, entry->cve_id, entry->repo_name, entry->commit_hash);
    unsigned line_number = history_add_line(history, history->parsing_section, prefix, "parsing...");
    free(prefix);

    int fd = open(output_path, O_WRONLY | O_CREAT | O_EXCL, 0644);
    free(output_path);

    if (fd == -1 && errno == EEXIST) {
        history_update_line(history, history->parsing_section, line_number, "file already exists", true);
        dataset_entry_destroy(entry);
        return;
    }

    history_update_line(history, history->parsing_section, line_number, "parsing...", false);

    EXIT_IF(fd == -1, "open");
    FILE *f = fdopen(fd, "w");
    EXIT_IF(f == NULL, "fdopen");

    json_write(f, 0, "{\n");
    json_write(f, 1, "\"CWE\": \"CWE-%u\",\n", entry->cwe_id);
    json_write(f, 1, "\"CVE\": \"%s\",\n", entry->cve_id);
    json_write(f, 1, "\"repository\": \"%s\",\n", entry->repo_name);
    json_write(f, 1, "\"commit\": \"%s\",\n", entry->commit_hash);

    json_write(f, 1, "\"files\": [\n");
    for (unsigned i = 0; i < entry->files_count; i++) {
        json_write(f, 2, "{\n");

        json_write(f, 3, "\"path\": \"%s\"\n", entry->files[i]->path);

        if (i < entry->files_count - 1)
            json_write(f, 2, "},\n");
        else
            json_write(f, 2, "}\n");
    }
    json_write(f, 1, "]\n");

    json_write(f, 0, "}\n");

    EXIT_IF(fclose(f) == EOF, "fclose");

    history_update_line(history, history->parsing_section, line_number, "complete", true);
    dataset_entry_destroy(entry);
}
