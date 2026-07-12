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

    switch (pid) {
    case -1:
        exit_if(1, __func__, "fork");
        break;

    case 0:
        exit_if(execvp(argv[0], argv) == -1, __func__, "execvp");
        break;

    default:
        exit_if(waitpid(pid, &status, 0) == -1, __func__, "waitpid");
        exit_if(!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS, __func__, argv[0]);
    }
}

void remove_directory(char *path) {
    char *cmd[] = {"rm", "-rf", path, NULL};
    execute(cmd);
}

void download_repository(char *source, char *target) {
    struct stat st;

    if (stat(target, &st) != -1)
        remove_directory(target);

    char *cmd[] = {"git", "clone", source, target, NULL};
    execute(cmd);
}
