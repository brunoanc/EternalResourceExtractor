#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <regex>
#include <filesystem>

namespace fs = std::filesystem;

void pressAnyKey();
void throwError(const std::string &error);
std::string formatPath(std::string path);
std::vector<std::string> splitString(std::string stringToSplit, const char delimiter);
int mkpath(const fs::path &filePath, size_t startPos);
void compileRegexes(std::vector<std::regex> &regexesToMatch, std::vector<std::regex> &regexesNotToMatch, const std::vector<std::pair<std::string, std::string>> &params);

#endif
