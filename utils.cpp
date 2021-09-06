#include <iostream>
#include <string>
#include <vector>
#include "utils.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <conio.h>
#endif

#ifdef _WIN32
// Check if process is running on a terminal by checking the console processes
inline bool isRunningOnTerminal()
{
    DWORD buffer[1];
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

// Split the given string using the given delimiter
std::vector<std::string> splitString(std::string stringToSplit, const char delimiter)
{
    std::vector<std::string> resultVector;
    size_t pos;
    std::string part;

    while ((pos = stringToSplit.find(delimiter)) != std::string::npos) {
        part = stringToSplit.substr(0, pos);
        resultVector.push_back(part);
        stringToSplit.erase(0, pos + 1);
    }

    resultVector.push_back(stringToSplit);

    return resultVector;
}
