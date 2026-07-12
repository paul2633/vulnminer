#ifndef CONFIG_H
#define CONFIG_H

typedef enum { MODE_UNKNOWN = 0, MODE_LOCAL, MODE_DOWNLOAD, MODE_REMOTE } repository_mode_t;

typedef enum { GRANULARITY_FILE = 0, GRANULARITY_FUNCTION, GRANULARITY_LINE } granularity_t;

typedef struct {
    repository_mode_t mode;
    char *source;
    char *commit;
    unsigned threads;
    granularity_t granularity;
} config_t;

void config_init(config_t *config, int argc, char **argv);

void config_destroy(config_t *config);

#endif
