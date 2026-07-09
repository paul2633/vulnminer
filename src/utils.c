#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "repository.h"
#include "utils.h"

void exit_if(int condition, const char *msg) {
    if (condition) {
        fprintf(stderr, ERROR_C "[ERROR] %s" RESET_C "\n", msg);
        exit(EXIT_FAILURE);
    }
}

static void execute(char *const argv[]) {
    int status;
    pid_t pid = fork();

    switch (pid) {
    case -1:
        exit_if(1, "fork");
        break;

    case 0:
        exit_if(execvp(argv[0], argv) == -1, "execvp");
        break;

    default:
        exit_if(waitpid(pid, &status, 0) == -1, "waitpid");
        exit_if(!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS, argv[0]);
    }
}

void remove_directory(char *path) {
    char *cmd[] = {"rm", "-rf", path, NULL};
    execute(cmd);
}

void download_repository(const repository_t *repo) {
    struct stat st;

    if (stat(repo->name, &st) != -1)
        remove_directory(repo->name);

    char *cmd[] = {"git", "clone", repo->url, repo->name, NULL};
    execute(cmd);
}
