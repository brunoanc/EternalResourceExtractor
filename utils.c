#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>

#include "utils.h"

#ifdef _WIN32
#include <Windows.h>
#include <conio.h>
#include <direct.h>
#define mkdir(x) _wmkdir(x)
#else
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#define mkdir(x) mkdir(x, 0777)
#endif

#ifdef _WIN32
bool check_terminal(void)
{
    DWORD *buffer = malloc(sizeof(DWORD));

    if (!buffer)
        return false;

    DWORD count = GetConsoleProcessList(buffer, 1);
    free(buffer);

    if (count == 1) {
        return false;
    }
    else {
        return true;
    }
}

wchar_t *char_to_wchar(char *path)
{
    long wstr_len = MultiByteToWideChar(CP_UTF8, 0, path, -1, NULL, 0);

    if (wstr_len == 0)
        return NULL;

    wchar_t *path_wstr = malloc(wstr_len * sizeof(wchar_t));

    if (!path_wstr)
        return NULL;

    if (MultiByteToWideChar(CP_UTF8, 0, path, -1, path_wstr, wstr_len) == 0)
        return NULL;

    return path_wstr;
}
#endif

void press_any_key(void)
{
#ifdef _WIN32
    if (check_terminal())
        return;

    printf("\nPress any key to continue...\n");
    getch();
#endif
}


void change_separator(char *path)
{
#ifdef _WIN32
    for (int i = 0; i < strlen(path); i++) {
        if (path[i] == '/')
            path[i] = '\\';
    }
#endif
}

int mkpath(void *file_path, unsigned long start_pos)
{
#ifdef _WIN32
    wchar_t *path = file_path;
    for (wchar_t *p = wcschr(path + start_pos, separator_char); p; p = wcschr(p + 1, separator_char)) {
#else

    char *path = file_path;
    for (char *p = strchr(path + start_pos, separator_char); p; p = strchr(p + 1, separator_char)) {
#endif

        *p = '\0';

        if (mkdir(path) == -1 && errno != EEXIST) {
            *p = separator_char;
            return -1;
        }

        *p = separator_char;
    }

    return 0;
}

char *fmt_path(char *str)
{
    char *newline = strchr(str, '\n');

    if (newline)
        *newline = '\0';

    if (*str == '"')
        str = str + 1;
    
    if (*(str + strlen(str) - 1) == '"')
        *(str + strlen(str) - 1) = '\0';
    
    char *end;

    while (isspace((unsigned char)*str))
        str++;

    if (*str == '\0')
        return str;

    end = str + strlen(str) - 1;
    
    while (end > str && isspace((unsigned char)*end))
        end--;

    end[1] = '\0';

    return str;
}