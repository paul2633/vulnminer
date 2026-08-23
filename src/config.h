#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char *cve_published_before_str;
    time_t cve_published_before;
    char *cve_published_after_str;
    time_t cve_published_after;
    unsigned max_commits_per_cwe;
    char *nvd_api_key;
    char *cwe_ids_raw;
    unsigned *cwe_ids;
    unsigned cwe_ids_count;
    unsigned threads_count;
} config_t;

void config_init(config_t *config, int argc, char **argv);

void config_destroy(config_t *config);

#endif
