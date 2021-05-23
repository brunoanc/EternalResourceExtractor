#include <stdio.h>

#ifndef UTILS_H
#define UTILS_H

#ifdef _WIN32
#define separator_char '\\'
wchar_t *char_to_wchar(char *path);
#else
#define separator_char '/'
#endif

void press_any_key();
void change_separator(char *path);
int mkpath(void *file_path, unsigned long start_pos);
char *fmt_path(char *str);
ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp);

#endif