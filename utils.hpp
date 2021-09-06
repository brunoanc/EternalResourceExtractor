#ifndef UTILS_H
#define UTILS_H

#include <filesystem>

namespace fs = std::filesystem;

void pressAnyKey();
void throwError(const std::string& error);
std::string formatPath(std::string path);
std::vector<std::string> splitString(std::string stringToSplit, const char delimiter);

#endif
