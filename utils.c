#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "utils.h"

#ifdef _WIN32
#include <Windows.h>
#include <conio.h>
#include <direct.h>
#define mkdir(x) _mkdir(x)
#else
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#define mkdir(x) mkdir(x, 0777)
#endif

#ifdef _WIN32
bool __cdecl check_terminal()
{
    DWORD *buffer = malloc(sizeof(DWORD));
    DWORD count = GetConsoleProcessList(buffer, 1);

    if (count == 1) {
        return false;
    }
    else {
        return true;
    }
}
#endif

void press_any_key()
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

int mkpath(char *file_path)
{
    for (char *p = strchr(file_path + 1, separator_char); p; p = strchr(p + 1, separator_char)) {
        *p = '\0';

        if (mkdir(file_path) == -1 && errno != EEXIST) {
            *p = separator_char;
            return -1;
        }

        *p = separator_char;
    }

    return 0;
}

void remove_quotes_and_newline(char **str)
{
    char *newline = strchr(*str, '\n');

    if (newline)
        *newline = '\0';

    if (**str == '"')
        *str = *str + 1;
    
    if (*(*str + strlen(*str) - 1) == '"')
        *(*str + strlen(*str) - 1) = '\0';
}