#ifndef UTILS_H
#define UTILS_H

#define RESET_C "\033[0m"
#define ERROR_C "\033[0m\033[31m"

void exit_if(int condition, const char *fn_name, const char *msg);

void remove_directory(char *path);

void download_repository(char *source, char *target);

#endif
