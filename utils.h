#include <stdio.h>

#ifndef UTILS_H
#define UTILS_H

#ifdef _WIN32
#define separator_char '\\'
#elif defined __linux__
#define separator_char '/'
#endif

void press_any_key();
void change_separator(char *path);
int mkpath(char *file_path);
void remove_quotes_and_newline(char **str);
ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp);

#endif