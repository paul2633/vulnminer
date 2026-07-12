#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
                                        {NULL, 0, NULL, 0}};

static void parse_mode(config_t *config, const char *value) {
    if (strcmp(value, "local") == 0)
        config->mode = MODE_LOCAL;
    else if (strcmp(value, "download") == 0)
        config->mode = MODE_DOWNLOAD;
    else if (strcmp(value, "remote") == 0)
        config->mode = MODE_REMOTE;
}

static void parse_granularity(config_t *config, const char *value) {
    if (strcmp(value, "file") == 0)
        config->granularity = GRANULARITY_FILE;
    else if (strcmp(value, "function") == 0)
        config->granularity = GRANULARITY_FUNCTION;
    else if (strcmp(value, "line") == 0)
        config->granularity = GRANULARITY_LINE;
}

static void set_string(char **source, const char *new) {
    free(*source);
    *source = strdup(new);
    exit_if(*source == NULL, __func__, "strdup");
}

static void parse_config_file(config_t *config, const char *path) {
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
    }

    exit_if(fclose(f) == EOF, __func__, "fclose");
}

static void parse_options(config_t *config, int argc, char **argv) {
    int opt;

    while ((opt = getopt_long(argc, argv, "o:m:s:c:t:g:", options, NULL)) != -1) {

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
    }
}

void config_init(config_t *config, int argc, char **argv) {
    memset(config, 0, sizeof(*config));
    config->threads = 1;

    int opt;

    while ((opt = getopt_long(argc, argv, "o:m:s:c:t:g:", options, NULL)) != -1)
        if (opt == 'o')
            parse_config_file(config, optarg);

    optind = 1;
    parse_options(config, argc, argv);

    exit_if(config->mode == MODE_UNKNOWN, __func__, "invalid mode");
    exit_if(config->source == NULL, __func__, "invalid source");
    exit_if(config->threads == 0, __func__, "invalid threads");

    size_t len = strlen(config->source);
    if (len >= 4 && strcmp(config->source + len - 4, ".git") == 0)
        config->source[len - 4] = '\0';
}

void config_destroy(config_t *config) {
    free(config->source);
    free(config->commit);
}
