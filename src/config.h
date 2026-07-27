#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    const char *name;
    int value;
} option_map_t;

typedef enum { OPT_STRING, OPT_UINT, OPT_ENUM } option_type_t;

typedef struct {
    char shortopt;
    const char *longopt;
    option_type_t type;
    size_t offset;
    const option_map_t *map;
} option_desc_t;

typedef enum { MODE_UNKNOWN = 0, MODE_LOCAL, MODE_DOWNLOAD, MODE_REMOTE } repository_mode_t;

static const option_map_t mode_map[] = {{"local", MODE_LOCAL}, {"download", MODE_DOWNLOAD}, {"remote", MODE_REMOTE}, {NULL, 0}};

typedef enum { GRANULARITY_FILE = 0, GRANULARITY_FUNCTION, GRANULARITY_LINE } granularity_t;

static const option_map_t granularity_map[] = {{"file", GRANULARITY_FILE}, {"function", GRANULARITY_FUNCTION}, {"line", GRANULARITY_LINE}, {NULL, 0}};

typedef enum { VARIANT_SEQ = 0, VARIANT_OMP_FOR, VARIANT_OMP_TASK } variant_t;

static const option_map_t variant_map[] = {{"seq", VARIANT_SEQ}, {"omp_for", VARIANT_OMP_FOR}, {"omp_task", VARIANT_OMP_TASK}, {NULL, 0}};

typedef struct {
    repository_mode_t mode;
    char *source;
    char *commit;
    unsigned threads;
    granularity_t granularity;
    char *exclude_raw;
    char **exclude;
    variant_t variant;
    char *json_path;
    char *perfs_path;
} config_t;

const char *enum_to_string(const option_map_t *map, int value);

void config_init(config_t *config, int argc, char **argv);

void config_destroy(config_t *config);

#endif
