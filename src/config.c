#include <getopt.h>
#include <omp.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config.h"
#include "utils.h"

#define CONFIG_OPTIONS(X)                                                                                                                                      \
    X(mode, 'm', "m", "mode", OPT_ENUM, mode_map)                                                                                                              \
    X(source, 's', "s", "source", OPT_STRING, NULL)                                                                                                            \
    X(commit, 'c', "c", "commit", OPT_STRING, NULL)                                                                                                            \
    X(granularity, 'g', "g", "granularity", OPT_ENUM, granularity_map)                                                                                         \
    X(exclude_raw, 'e', "e", "exclude", OPT_STRING, NULL)                                                                                                      \
    X(variant, 'v', "v", "variant", OPT_ENUM, variant_map)                                                                                                     \
    X(threads, 't', "t", "threads", OPT_UINT, NULL)                                                                                                            \
    X(json_path, 'j', "j", "json-path", OPT_STRING, NULL)                                                                                                      \
    X(perfs_path, 'p', "p", "perfs-path", OPT_STRING, NULL)

#define X(field, shortopt, shortstr, longopt, type, map) {longopt, required_argument, NULL, shortopt},

static const struct option options[] = {CONFIG_OPTIONS(X){"options-file", required_argument, NULL, 'o'}, {NULL, 0, NULL, 0}};

#undef X

#define X(field, shortopt, shortstr, longopt, type, map) shortstr ":"

static const char short_options[] = "o:" CONFIG_OPTIONS(X);

#undef X

#define X(field, shortopt, shortstr, longopt, type, map) {shortopt, longopt, type, offsetof(config_t, field), map},

static const option_desc_t option_desc[] = {CONFIG_OPTIONS(X){0, NULL, 0, 0, NULL}};

#undef X

const char *enum_to_string(const option_map_t *map, int value) {
    for (size_t i = 0; map[i].name != NULL; i++) {
        if (map->value == value) {
            return map->name;
        }
    }

    return "unknown";
}

static void set_option(config_t *config, const option_desc_t *desc, const char *optarg) {
    void *dst = (char *)config + desc->offset;

    switch (desc->type) {
    case OPT_STRING:
        if (optarg[0] == '\0')
            return;
        *(char **)dst = strdup(optarg);
        exit_if((char *)dst == NULL, __func__, "strdup");
        break;

    case OPT_UINT:
        *(unsigned *)dst = atoi(optarg);
        break;

    case OPT_ENUM:
        for (size_t i = 0; desc->map[i].name != NULL; i++) {
            if (strcmp(desc->map[i].name, optarg) == 0) {
                *(int *)dst = desc->map[i].value;
                break;
            }
        }
        break;
    }
}

static void exclude_str_to_list(config_t *config) {
    if (config->exclude_raw == NULL)
        return;

    size_t count = 1;

    for (size_t i = 0; config->exclude_raw[i] != '\0'; i++)
        if (config->exclude_raw[i] == ',')
            count++;

    config->exclude = malloc(sizeof(*config->exclude) * (count + 1));
    exit_if(config->exclude == NULL, __func__, "malloc");

    config->exclude[0] = config->exclude_raw;
    config->exclude[count] = NULL;

    for (size_t i = 0, j = 1; config->exclude_raw[i] != '\0'; i++) {
        if (config->exclude_raw[i] == ',') {
            config->exclude_raw[i] = '\0';
            config->exclude[j++] = config->exclude_raw + i + 1;
        }
    }
}

static void parse_config_file(config_t *config, const char *path) {
    FILE *f = fopen(path, "r");
    exit_if(f == NULL, __func__, "fopen");

    char line[256];

    while (fgets(line, sizeof(line), f)) {

        line[strcspn(line, "\n")] = '\0';

        if (line[0] == '\0' || line[0] == '#')
            continue;

        char *value = strchr(line, '=');
        if (value == NULL)
            continue;

        *value++ = '\0';

        for (size_t i = 0; option_desc[i].longopt != NULL; i++) {
            if (strcmp(line, option_desc[i].longopt) == 0) {
                set_option(config, &option_desc[i], value);
                break;
            }
        }
    }

    exit_if(fclose(f) == EOF, __func__, "fclose");
}

void config_init(config_t *config, int argc, char **argv) {
    memset(config, 0, sizeof(*config));

    int opt;
    while ((opt = getopt_long(argc, argv, short_options, options, NULL)) != -1) {
        if (opt == 'o') {
            parse_config_file(config, optarg);
        }
    }

    optind = 1;
    int index;
    while ((opt = getopt_long(argc, argv, short_options, options, &index)) != -1) {
        if (opt != 'o') {
            set_option(config, option_desc + index, optarg);
        }
    }

    config->threads = config->variant == VARIANT_SEQ || config->threads == 0 ? 1 : config->threads;
    omp_set_num_threads(config->threads);

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

    exclude_str_to_list(config);

    printf(LOG_C "[INIT] mode: %s\n", enum_to_string(mode_map, config->mode));
    printf("[INIT] source: %s\n", config->source);
    printf("[INIT] commit: %s\n", config->commit);

    printf("[INIT] granularity: %s\n", enum_to_string(granularity_map, config->granularity));
    printf("[INIT] excluded directories: ");
    print_str_list(config->exclude);

    printf("\n[INIT] variant: %s\n", enum_to_string(variant_map, config->variant));
    printf("[INIT] threads number: %u\n", config->threads);

    printf("[INIT] json-path: %s\n", config->json_path);
    printf("[INIT] perfs-path: %s" RESET_C "\n\n", config->perfs_path);
}

void config_destroy(config_t *config) {
    free(config->source);
    free(config->commit);
    free(config->exclude_raw);
    free(config->exclude);
    free(config->json_path);
    free(config->perfs_path);
}
