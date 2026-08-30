#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <time.h>

typedef struct {
    char *cwe_ids_raw;
    unsigned *cwe_ids;
    unsigned cwe_ids_count;

    char *cve_published_before_str;
    time_t cve_published_before;
    char *cve_published_after_str;
    time_t cve_published_after;

    char *nvd_api_key;
    char *github_api_key;

    bool include_c_files;
    bool include_cpp_files;
} config_t;

config_t *config_new(int argc, char **argv);

void config_destroy(config_t *config);

void display_config(const config_t *config);

#endif
