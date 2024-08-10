#ifndef WAD7_HPP
#define WAD7_HPP

#include <vector>
#include <regex>
#include "mmap/mmap.hpp"

size_t extractResource(MemoryMappedFile *memoryMappedFile, const std::string &outPath, const std::vector<std::regex> &regexesToMatch, const std::vector<std::regex> &regexesNotToMatch);
size_t extractWad7(MemoryMappedFile *memoryMappedFile, const std::string &outPath, const std::vector<std::regex> &regexesToMatch, const std::vector<std::regex> &regexesNotToMatch);

#endif
