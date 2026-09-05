#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "utils.h"

#define MATCH(key) (strncmp(line, key "=", sizeof(key)) == 0)
#define VALUE(key) (line + sizeof(key))

static const struct option options[] = {{"options-file", required_argument, NULL, 'i'}, {NULL, 0, NULL, 0}};

static void set_string(char **dst, const char *new) {
    free(*dst);

    if (new[0] == '\0') {
        *dst = NULL;
        return;
    }

    *dst = strdup(new);
    EXIT_IF(*dst == NULL, "strdup");
}

static void date_to_display(char *dst, size_t size, time_t date) {
    struct tm tm = {0};
    EXIT_IF(gmtime_r(&date, &tm) == NULL, "gmtime_r");
    EXIT_IF(strftime(dst, size, "%d/%m/%Y", &tm) == 0, "strftime");
}

static void parse_dates(config_t *config) {
    struct tm tm = {0};
    if (config->cve_published_before_str == NULL || strptime(config->cve_published_before_str, "%d/%m/%Y", &tm) == NULL)
        config->cve_published_before = time(NULL);
    else
        config->cve_published_before = timegm(&tm);

    EXIT_IF(config->cve_published_before == (time_t)-1, "timegm");
    EXIT_IF(gmtime_r(&config->cve_published_before, &tm) == NULL, "gmtime_r");

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

    char before_display[sizeof("DD/MM/YYYY")], after_display[sizeof("DD/MM/YYYY")];
    date_to_display(before_display, sizeof(before_display), config->cve_published_before);
    date_to_display(after_display, sizeof(after_display), config->cve_published_after);
    set_string(&config->cve_published_before_str, before_display);
    set_string(&config->cve_published_after_str, after_display);
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

static void parse_config_file(config_t *config, const char *path) {
    FILE *f = fopen(path, "r");
    EXIT_IF(f == NULL, "fopen");

    char line[256];

    while (fgets(line, sizeof(line), f)) {

        line[strcspn(line, "\n")] = '\0';
        if (line[0] == '\0' || line[0] == '#')
            continue;

        if (MATCH("cwe-ids"))
            set_string(&config->cwe_ids_raw, VALUE("cwe-ids"));
        else if (MATCH("cve-published-before"))
            set_string(&config->cve_published_before_str, VALUE("cve-published-before"));
        else if (MATCH("cve-published-after"))
            set_string(&config->cve_published_after_str, VALUE("cve-published-after"));
        else if (MATCH("nvd-api-key"))
            set_string(&config->nvd_api_key, VALUE("nvd-api-key"));
        else if (MATCH("github-api-key"))
            set_string(&config->github_api_key, VALUE("github-api-key"));
        else if (MATCH("include-c-files"))
            config->include_c_files = strcmp(VALUE("include-c-files"), "yes") == 0;
        else if (MATCH("include-cpp-files"))
            config->include_cpp_files = strcmp(VALUE("include-cpp-files"), "yes") == 0;
    }

    EXIT_IF(fclose(f) == EOF, "fclose");
}

config_t *config_new(int argc, char **argv) {
    config_t *config = calloc(1, sizeof(*config));
    EXIT_IF(config == NULL, "calloc");

    const char *config_path = "../config.ini";
    const char *export_folder_path = "../results";

    int opt;
    while ((opt = getopt_long(argc, argv, "i:o:", options, NULL)) != -1) {
        if (opt == 'i')
            config_path = optarg;
        if (opt == 'o')
            export_folder_path = optarg;
    }

    config->export_folder_path = realpath(export_folder_path, NULL);
    EXIT_IF(config->export_folder_path == NULL, "realpath");

    struct stat st;
    EXIT_IF(stat(config->export_folder_path, &st) == -1, "stat");
    EXIT_IF(!S_ISDIR(st.st_mode), "not a directory");

    parse_config_file(config, config_path);

    EXIT_IF(config->github_api_key == NULL, "GitHub API key is required");

    parse_cwe_ids(config);
    parse_dates(config);

    return config;
}

void config_destroy(config_t *config) {
    free(config->cwe_ids_raw);
    free(config->cwe_ids);
    free(config->cve_published_before_str);
    free(config->cve_published_after_str);
    free(config->nvd_api_key);
    free(config->github_api_key);
    free(config->export_folder_path);
    free(config);
}

void display_config(const config_t *config) {
    printf(LOG_C "[INIT] CWE IDs: ");
    for (unsigned i = 0; i < config->cwe_ids_count; i++) {
        printf("%u", config->cwe_ids[i]);
        if (i < config->cwe_ids_count - 1)
            printf(", ");
    }
    printf(RESET_C "\n");
    printf(LOG_C "[INIT] CVEs published before: %s" RESET_C "\n", config->cve_published_before_str);
    printf(LOG_C "[INIT] CVEs published after: %s" RESET_C "\n", config->cve_published_after_str);
    printf("\n");
    printf(LOG_C "[INIT] NVD API key: %s" RESET_C "\n", config->nvd_api_key);
    printf(LOG_C "[INIT] GitHub API key: %s" RESET_C "\n", config->github_api_key);
    printf("\n");
    printf(LOG_C "[INIT] include C files: %s" RESET_C "\n", config->include_c_files ? "yes" : "no");
    printf(LOG_C "[INIT] include C++ files: %s" RESET_C "\n", config->include_cpp_files ? "yes" : "no");
    printf("\n");
    printf(LOG_C "[INIT] export folder path: %s" RESET_C "\n", config->export_folder_path);
}
