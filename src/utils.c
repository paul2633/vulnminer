#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utils.h"

void exit_if(int condition, const char *fn_name, const char *msg) {
    if (condition) {
        fprintf(stderr, ERROR_C "[ERROR] (%s) %s" RESET_C "\n", fn_name, msg);
        exit(EXIT_FAILURE);
    }
}

static void execute(char *const argv[]) {
    int status;
    pid_t pid = fork();

    exit_if(pid == -1, __func__, "fork");

    if (pid == 0) {
        exit_if(execvp(argv[0], argv) == -1, __func__, "execvp");
    }

    else {
        exit_if(waitpid(pid, &status, 0) == -1, __func__, "waitpid");
        exit_if(!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS, __func__, argv[0]);
    }
}

void remove_directory(char *path) {
    char *rm[] = {"rm", "-rf", path, NULL};
    execute(rm);
}

void download_repository(char *source, char *commit, char *target) {
    struct stat st;

    if (stat(target, &st) != -1)
        remove_directory(target);

    char *clone[] = {"git", "clone", source, target, NULL};
    execute(clone);

    if (commit != NULL) {
        char *checkout[] = {"git", "-C", target, "checkout", commit, NULL};
        execute(checkout);
    }
}

void json_write(FILE *f, unsigned indent, const char *fmt, ...) {
    for (unsigned i = 0; i < indent; i++)
        exit_if(fputs("    ", f) == EOF, __func__, "fputs");

    va_list ap;
    va_start(ap, fmt);
    exit_if(vfprintf(f, fmt, ap) < 0, __func__, "vfprintf");
    va_end(ap);
}
