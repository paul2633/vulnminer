#ifndef CONFIG_H
#define CONFIG_H

typedef enum { MODE_UNKNOWN = 0, MODE_LOCAL, MODE_DOWNLOAD, MODE_REMOTE } repository_mode_t;

typedef enum { GRANULARITY_FILE = 0, GRANULARITY_FUNCTION, GRANULARITY_LINE } granularity_t;

typedef enum { VARIANT_SEQ = 0, VARIANT_OMP_FOR, VARIANT_OMP_TASK } variant_t;

typedef struct {
    repository_mode_t mode;
    char *source;
    char *commit;
    unsigned threads;
    granularity_t granularity;
    char **exclude;
    variant_t variant;
    char *json_path;
    char *perfs_path;
} config_t;

const char *mode_to_string(repository_mode_t mode);

const char *granularity_to_string(granularity_t granularity);

const char *variant_to_string(variant_t variant);

void config_init(config_t *config, int argc, char **argv);

void config_destroy(config_t *config);

#endif
