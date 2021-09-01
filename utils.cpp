#include <iostream>
#include <string>
#include "utils.hpp"
#include "oo2core.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <conio.h>
#else
#include <dlfcn.h>
#include "linoodle.hpp"
#endif

#ifdef _WIN32
// Check if process is running on a terminal by checking the console processes
inline bool isRunningOnTerminal()
{
    DWORD buffer[sizeof(DWORD)];
    return GetConsoleProcessList(buffer, 1) > 1;
}
#endif

// Display the 'press any key to exit' if process is not running in a terminal (Windows only)
void pressAnyKey()
{
#ifdef _WIN32
    if (isRunningOnTerminal())
        return;

    std::cout << "\nPress any key to exit..." << std::endl;
    _getch();
#endif
}

// Display error and exit
void throwError(const std::string& error)
{
    std::cout.flush();
    std::cerr << "\nERROR: " << error << std::endl;
    pressAnyKey();
    exit(1);
}

// Format path by removing whitespace and double quotes
std::string formatPath(std::string path)
{
    // Trim whitespace and quotes in string
    path.erase(path.find_last_not_of(" \t\n\r\f\v\"") + 1);
    path.erase(0, path.find_first_not_of(" \t\n\r\f\v\""));
    return path;
}

// Load oodle dll
bool oodleInit(OodLZ_DecompressFunc **OodLZ_Decompress)
{
    if (OodLZ_Decompress == nullptr)
        return false;

    // If oodle is not found, write it
    if (!fs::exists("oo2core_8_win64.dll")) {
        FILE *oo2core = fopen("oo2core_8_win64.dll", "wb");

        if (oo2core == nullptr)
            return false;

        fwrite(oo2coreDll, 1, sizeof(oo2coreDll), oo2core);
        fclose(oo2core);
    }

#ifdef _WIN32
    // Load OodleLZ_Decompress from oodle
    auto oodle = LoadLibraryA("./oo2core_8_win64.dll");
    *OodLZ_Decompress = reinterpret_cast<OodLZ_DecompressFunc*>(GetProcAddress(oodle, "OodleLZ_Decompress"));
#else

    // If linoodle is not found, write it
    if (!fs::exists("liblinoodle.so")) {
        FILE *linoodle = fopen("liblinoodle.so", "wb");

        if (linoodle == nullptr)
            return false;

        fwrite(linoodleLib, 1, sizeof(linoodleLib), linoodle);
        fclose(linoodle);
    }

    // Load OodleLZ_Decompress from linoodle
    void *oodle = dlopen("./liblinoodle.so", RTLD_LAZY);
    *OodLZ_Decompress = reinterpret_cast<OodLZ_DecompressFunc*>(dlsym(oodle, "OodleLZ_Decompress"));
#endif

    if (*OodLZ_Decompress == nullptr)
        return false;

    return true;
}
