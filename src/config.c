#include <getopt.h>
#include <omp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "config.h"
#include "utils.h"

#define MATCH(key) (strncmp(line, key "=", sizeof(key)) == 0)
#define VALUE(key) (line + sizeof(key))

static const struct option options[] = {{"options-file", required_argument, NULL, 'o'},
                                        {"cve-published-before", required_argument, NULL, 'b'},
                                        {"cve-published-after", required_argument, NULL, 'a'},
                                        {"max-commits-per-cwe", required_argument, NULL, 'c'},
                                        {"nvd-api-key", required_argument, NULL, 'k'},
                                        {"cwe-ids", required_argument, NULL, 'i'},
                                        {"threads-count", required_argument, NULL, 't'},
                                        {NULL, 0, NULL, 0}};

static void set_string(char **dst, const char *new) {
    free(*dst);

    if (new[0] == '\0') {
        *dst = NULL;
        return;
    }

    *dst = strdup(new);
    EXIT_IF(*dst == NULL, "strdup");
}

static void parse_config_file(config_t *config, const char *path) {
    FILE *f = fopen(path, "r");
    EXIT_IF(f == NULL, "fopen");

    char line[256];

    while (fgets(line, sizeof(line), f)) {

        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0' || line[0] == '#')
            continue;

        if (MATCH("cve-published-before"))
            set_string(&config->cve_published_before_str, VALUE("cve-published-before"));
        else if (MATCH("cve-published-after"))
            set_string(&config->cve_published_after_str, VALUE("cve-published-after"));
        else if (MATCH("max-commits-per-cwe"))
            config->max_commits_per_cwe = atoi(VALUE("max-commits-per-cwe"));
        else if (MATCH("nvd-api-key"))
            set_string(&config->nvd_api_key, VALUE("nvd-api-key"));
        else if (MATCH("cwe-ids"))
            set_string(&config->cwe_ids_raw, VALUE("cwe-ids"));
        else if (MATCH("threads-count"))
            config->threads_count = atoi(VALUE("threads-count"));
    }

    EXIT_IF(fclose(f) == EOF, "fclose");
}

static void parse_options(config_t *config, int argc, char **argv) {
    int opt;

    while ((opt = getopt_long(argc, argv, "o:b:a:c:k:i:t:", options, NULL)) != -1) {

        if (opt == 'o')
            parse_config_file(config, optarg);
        else if (opt == 'b')
            set_string(&config->cve_published_before_str, optarg);
        else if (opt == 'a')
            set_string(&config->cve_published_after_str, optarg);
        else if (opt == 'c')
            config->max_commits_per_cwe = atoi(optarg);
        else if (opt == 'k')
            set_string(&config->nvd_api_key, optarg);
        else if (opt == 'i')
            set_string(&config->cwe_ids_raw, optarg);
        else if (opt == 't')
            config->threads_count = atoi(optarg);
    }
}

static void parse_dates(config_t *config) {
    struct tm tm = {0};
    time_t cve_published_before;
    if (config->cve_published_before_str == NULL || strptime(config->cve_published_before_str, "%d/%m/%Y", &tm) == NULL)
        cve_published_before = time(NULL);
    else
        cve_published_before = timegm(&tm);
    EXIT_IF(cve_published_before == (time_t)-1, "timegm");
    EXIT_IF(gmtime_r(&cve_published_before, &tm) == NULL, "gmtime_r");
    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 59;
    config->cve_published_before = timegm(&tm);
    EXIT_IF(config->cve_published_before == (time_t)-1, "timegm");

    tm = (struct tm){0};
    if (config->cve_published_after_str == NULL || strptime(config->cve_published_after_str, "%d/%m/%Y", &tm) == NULL) {
        tm = (struct tm){0};
        EXIT_IF(strptime("01/01/1999", "%d/%m/%Y", &tm) == NULL, "strptime");
    }
    config->cve_published_after = timegm(&tm);
    EXIT_IF(config->cve_published_after == (time_t)-1, "timegm");

    char cve_published_before_str[11], cve_published_after_str[11];
    date_to_display_format(cve_published_before_str, sizeof(cve_published_before_str), config->cve_published_before);
    date_to_display_format(cve_published_after_str, sizeof(cve_published_after_str), config->cve_published_after);
    set_string(&config->cve_published_before_str, cve_published_before_str);
    set_string(&config->cve_published_after_str, cve_published_after_str);
}

static void parse_cwe_ids(config_t *config) {
    if (config->cwe_ids_raw == NULL)
        return;

    config->cwe_ids_count = 1;
    for (int i = 0; config->cwe_ids_raw[i] != '\0'; i++)
        if (config->cwe_ids_raw[i] == ',')
            config->cwe_ids_count++;

    config->cwe_ids = malloc(sizeof(*config->cwe_ids) * config->cwe_ids_count);
    EXIT_IF(config->cwe_ids == NULL, "malloc");

    config->cwe_ids[0] = atoi(config->cwe_ids_raw);
    for (int i = 0, j = 1; config->cwe_ids_raw[i] != '\0'; i++) {
        if (config->cwe_ids_raw[i] == ',') {
            config->cwe_ids[j++] = atoi(config->cwe_ids_raw + i + 1);
        }
    }
}

void config_init(config_t *config, int argc, char **argv) {
    memset(config, 0, sizeof(*config));
    parse_options(config, argc, argv);

    parse_cwe_ids(config);
    EXIT_IF(config->cwe_ids_count == 0, "at least one CWE is required");
    parse_dates(config);
    config->threads_count = config->threads_count == 0 ? 1 : config->threads_count;
    omp_set_num_threads(config->threads_count);

    display_config(config);
}

void config_destroy(config_t *config) {
    free(config->cve_published_before_str);
    free(config->cve_published_after_str);
    free(config->nvd_api_key);
    free(config->cwe_ids_raw);
    free(config->cwe_ids);
}
