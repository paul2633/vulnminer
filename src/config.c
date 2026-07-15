#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>
#include <sys/stat.h>

#include "config.h"
#include "utils.h"

#define MATCH(key) (strncmp(line, key "=", sizeof(key)) == 0)
#define VALUE(key) (line + sizeof(key))

static const struct option options[] = {{"options-file", required_argument, NULL, 'o'}, 
                                        {"mode", required_argument, NULL, 'm'},
                                        {"source", required_argument, NULL, 's'},       
                                        {"commit", required_argument, NULL, 'c'},
                                        {"threads", required_argument, NULL, 't'},      
                                        {"granularity", required_argument, NULL, 'g'},
                                        {"exclude", required_argument, NULL, 'e'},      
                                        {"variant", required_argument, NULL, 'v'},
                                        {"json-path", required_argument, NULL, 'j'},
                                        {"perfs-path", required_argument, NULL, 'p'},
                                        {NULL, 0, NULL, 0}};

static void parse_mode(config_t *config, const char *value) {
    if (strcmp(value, "local") == 0)
        config->mode = MODE_LOCAL;
    else if (strcmp(value, "download") == 0)
        config->mode = MODE_DOWNLOAD;
    else if (strcmp(value, "remote") == 0)
        config->mode = MODE_REMOTE;
}

const char *mode_to_string(repository_mode_t mode) {
    if (mode == MODE_LOCAL)
        return "local";
    else if (mode == MODE_DOWNLOAD)
        return "download";
    else if (mode == MODE_REMOTE)
        return "remote";
    return "unknown";
}

static void parse_granularity(config_t *config, const char *value) {
    if (strcmp(value, "file") == 0)
        config->granularity = GRANULARITY_FILE;
    else if (strcmp(value, "function") == 0)
        config->granularity = GRANULARITY_FUNCTION;
    else if (strcmp(value, "line") == 0)
        config->granularity = GRANULARITY_LINE;
}

const char *granularity_to_string(granularity_t granularity) {
    if (granularity == GRANULARITY_FILE)
        return "file";
    else if (granularity == GRANULARITY_FUNCTION)
        return "function";
    else if (granularity == GRANULARITY_LINE)
        return "line";
    return "unknown";
}

static void parse_variant(config_t *config, const char *value) {
    if (strcmp(value, "seq") == 0)
        config->variant = VARIANT_SEQ;
    else if (strcmp(value, "omp_for") == 0)
        config->variant = VARIANT_OMP_FOR;
    else if (strcmp(value, "omp_task") == 0)
        config->variant = VARIANT_OMP_TASK;
}

const char *variant_to_string(variant_t variant) {
    if (variant == VARIANT_SEQ)
        return "seq";
    else if (variant == VARIANT_OMP_FOR)
        return "omp_for";
    else if (variant == VARIANT_OMP_TASK)
        return "omp_task";
    return "unknown";
}

static void set_string(char **dst, const char *new) {
    if (new[0] == '\0')
        return;
    if (*dst != NULL)
        free(*dst);
    *dst = strdup(new);
    exit_if(*dst == NULL, __func__, "strdup");
}

static void parse_config_file(config_t *config, const char *path, char **exclude) {
    FILE *f = fopen(path, "r");
    exit_if(f == NULL, __func__, "fopen");

    char line[256];

    while (fgets(line, sizeof(line), f)) {

        line[strcspn(line, "\n")] = '\0';

        if (line[0] == '\0' || line[0] == '#')
            continue;

        if (MATCH("mode"))
            parse_mode(config, VALUE("mode"));
        else if (MATCH("source"))
            set_string(&config->source, VALUE("source"));
        else if (MATCH("commit"))
            set_string(&config->commit, VALUE("commit"));
        else if (MATCH("threads"))
            config->threads = atoi(VALUE("threads"));
        else if (MATCH("granularity"))
            parse_granularity(config, VALUE("granularity"));
        else if (MATCH("exclude"))
            set_string(exclude, VALUE("exclude"));
        else if (MATCH("variant"))
            parse_variant(config, VALUE("variant"));
        else if (MATCH("json-path"))
            set_string(&config->json_path, VALUE("json-path"));
        else if (MATCH("perfs-path"))
            set_string(&config->perfs_path, VALUE("perfs-path"));
    }

    exit_if(fclose(f) == EOF, __func__, "fclose");
}

static void parse_options(config_t *config, int argc, char **argv, char **exclude) {
    int opt;

    while ((opt = getopt_long(argc, argv, "o:m:s:c:t:g:e:v:j:p:", options, NULL)) != -1) {

        if (opt == 'm')
            parse_mode(config, optarg);
        else if (opt == 's')
            set_string(&config->source, optarg);
        else if (opt == 'c')
            set_string(&config->commit, optarg);
        else if (opt == 't')
            config->threads = atoi(optarg);
        else if (opt == 'g')
            parse_granularity(config, optarg);
        else if (opt == 'e')
            set_string(exclude, optarg);
        else if (opt == 'v')
            parse_variant(config, optarg);
        else if (opt == 'j')
            set_string(&config->json_path, optarg);
        else if (opt == 'p')
            set_string(&config->perfs_path, optarg);
    }
}

void config_init(config_t *config, int argc, char **argv) {
    memset(config, 0, sizeof(*config));

    char *exclude = NULL;

    int opt;

    while ((opt = getopt_long(argc, argv, "o:m:s:c:t:g:e:v:j:p:", options, NULL)) != -1)
        if (opt == 'o')
            parse_config_file(config, optarg, &exclude);

    optind = 1;
    parse_options(config, argc, argv, &exclude);

    config->threads = config->threads <= 0 ? 1 : config->threads;

    exit_if(config->mode == MODE_UNKNOWN, __func__, "missing mode");
    exit_if(config->source == NULL, __func__, "missing source");
    exit_if(config->json_path == NULL, __func__, "missing json path");

    char *json = realpath(config->json_path, NULL);
    exit_if(json == NULL, __func__, "realpath");

    free(config->json_path);
    config->json_path = json;

    struct stat st;
    exit_if(stat(config->json_path, &st) == -1, __func__, "stat");
    exit_if(!S_ISDIR(st.st_mode), __func__, "json path is not a directory");

    if (config->mode == MODE_LOCAL) {
        char *path = realpath(config->source, NULL);
        exit_if(path == NULL, __func__, "realpath");
        free(config->source);
        config->source = path;
    }

    else if (config->mode == MODE_DOWNLOAD || config->mode == MODE_REMOTE) {
        size_t len = strlen(config->source);
        if (len >= 4 && strcmp(config->source + len - 4, ".git") == 0)
            config->source[len - 4] = '\0';
    }

    if (exclude != NULL) {

        size_t count = 1;

        for (size_t i = 0; exclude[i] != '\0'; i++)
            if (exclude[i] == ',')
                count++;

        config->exclude = malloc(sizeof(*config->exclude) * (count + 1));
        exit_if(config->exclude == NULL, __func__, "malloc");

        config->exclude[0] = exclude;
        config->exclude[count] = NULL;

        for (size_t i = 0, j = 1; exclude[i] != '\0'; i++) {
            if (exclude[i] == ',') {
                exclude[i] = '\0';
                config->exclude[j++] = exclude + i + 1;
            }
        }
    }

    omp_set_num_threads(config->threads);

    printf(LOG_C "[INIT] mode: %s\n", mode_to_string(config->mode));
    printf("[INIT] source: \"%s\"\n", config->source);
    printf("[INIT] commit: %s\n", config->commit);
    printf("[INIT] threads number: %d\n", config->threads);
    printf("[INIT] granularity: %s\n", granularity_to_string(config->granularity));
    printf("[INIT] excluded directories: [");
    if (config->exclude != NULL) {
        for (size_t i = 0; config->exclude[i] != NULL; i++) {
            printf("\"%s\"", config->exclude[i]);
            if (config->exclude[i + 1] != NULL)
                printf(", ");
        }
    }
    printf("]" RESET_C "\n");
}

void config_destroy(config_t *config) {
    free(config->source);
    free(config->commit);
    if (config->exclude != NULL) {
        free(*config->exclude);
        free(config->exclude);
    }
    free(config->json_path);
    free(config->perfs_path);
}
