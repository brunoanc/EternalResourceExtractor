#include <iostream>
#include <string>
#include <vector>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <regex>
#include "utils.hpp"
#include "oodle.hpp"
#include "mmap/mmap.hpp"
#include "argh/argh.h"

namespace chrono = std::chrono;

int main(int argc, char **argv)
{
    // Disable sync with stdio
    std::ios::sync_with_stdio(false);

    // Buffer stdout
    char buffer[8192];
    std::cout.rdbuf()->pubsetbuf(buffer, sizeof(buffer));

    std::cout << "EternalResourceExtractor v3.1.0 by PowerBall253\n\n";

    // Parse arguments
    argh::parser cmdl;
    cmdl.add_params({"-f", "--filter", "-r", "--regex"});
    cmdl.parse(argc, argv);

    if (cmdl[{"-h", "--help"}]) {
        std::cout << "Usage:\n";
        std::cout << "EternalResourceExtractor [path to .resources file] [out path] [options]\n\n";
        std::cout << "Options:\n\n";
        std::cout << "-h, --help\t\tDisplay this help message and exit\n\n";
        std::cout << "-q, --quiet\t\tSilences output during the extraction process.\n\n";
        std::cout << "-f, --filter=FILTERS\tIndicate a pattern the filename must match to be extracted, using\n"
            << "\t\t\t'*' for matching various characters and '?' to match exactly one.\n";
        std::cout << "\t\t\tYou can also prepend a '!' at the beginning of a filter to indicate it\n"
        << "\t\t\tmust not be matched, and separate various filters with a ';'.\n\n";
        std::cout << "-r, --regex=REGEXES\tSimilar to -f, but allows full regular expressions to be passed.\n\n";
        std::cout.flush();
        return 1;
    }

    if (cmdl[{"-q", "--quiet"}])
        std::cout.setstate(std::ios::failbit); // Makes cout not output anything

    // Get regexes to match/not match
    std::vector<std::regex> regexesToMatch;
    std::vector<std::regex> regexesNotToMatch;
    const std::string charsToEscape = "\\^$*+?.()|{}[]";

    for (const auto& param : cmdl.params()) {
        if (param.first == "r" || param.first == "regex") {
            for (const auto& regex : splitString(param.second, ';')) {
                // Push regex to vector
                try {
                    if (regex[0] == '!')
                        regexesNotToMatch.emplace_back(regex.substr(1), std::regex_constants::ECMAScript | std::regex_constants::optimize);
                    else
                        regexesToMatch.emplace_back(regex, std::regex_constants::ECMAScript | std::regex_constants::optimize);
                }
                catch (const std::exception& e) {
                    throwError("Failed to parse " + regex + " regular expression: " + e.what());
                }
            }
        }
        else if (param.first == "f" || param.first == "filter") {
            for (const auto& filter : splitString(param.second, ';')) {
                // Convert filter into valid regex
                std::string regex;

                for (const auto& c : filter) {
                    switch (c) {
                        case '?':
                            regex.push_back('.');
                            break;
                        case '*':
                            regex += ".*";
                            break;
                        default:
                            if (charsToEscape.find(c) != std::string::npos)
                                regex.push_back('\\'); // Escape character with backslash

                            regex.push_back(c);
                            break;
                    }
                }

                // Push regex to vector
                try {
                    if (regex[0] == '!')
                        regexesNotToMatch.emplace_back(regex.substr(1), std::regex_constants::ECMAScript | std::regex_constants::optimize);
                    else
                        regexesToMatch.emplace_back(regex, std::regex_constants::ECMAScript | std::regex_constants::optimize);
                }
                catch (const std::exception& e) {
                    throwError("Failed to parse " + filter + " filter: " + e.what());
                }
            }
        }
    }

    // Get resource & out path
    const std::vector<std::string> args = cmdl.pos_args();
    std::string resourcePath;
    std::string outPath;
    std::error_code ec;

    switch (args.size()) {
        case 1:
            std::cout << "Input the path to the .resources file: ";
            std::cout.flush();
            std::getline(std::cin, resourcePath);

            std::cout << "Input the path to the out directory: ";
            std::cout.flush();
            std::getline(std::cin, outPath);

            std::cout << '\n';
            break;
        case 2:
            resourcePath = args[1];

            std::cout << "Input the path to the out directory: ";
            std::cout.flush();
            std::getline(std::cin, outPath);

            std::cout << '\n';
            break;
        default:
            resourcePath = args[1];
            outPath = args[2];
            break;
    }

    // Get full path for resource & out directory
    if (resourcePath.empty())
        throwError("Resource file was not specified.");

    if (outPath.empty())
        throwError("Out directory was not specified.");

    resourcePath = fs::absolute(formatPath(resourcePath), ec).string();

    if (ec.value() != 0)
        throwError("Failed to get resource path: " + ec.message());

    outPath = fs::absolute(formatPath(outPath), ec).string();

    if (ec.value() != 0)
        throwError("Failed to get out path: " + ec.message());

    if (outPath[outPath.length() - 1] != fs::path::preferred_separator)
        outPath.push_back(fs::path::preferred_separator);

#ifdef _WIN32
    // "\\?\" alongside the wide string functions is used to bypass PATH_MAX
    // Check https://docs.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation?tabs=cmd for details
    outPath = "\\\\?\\" + outPath;
#endif

    // Time program
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();

    // Open the resource file
    MemoryMappedFile *memoryMappedFile;
    size_t memPosition = 0;

    try {
        memoryMappedFile = new MemoryMappedFile(resourcePath);
    }
    catch (const std::exception &e) {
        throwError("Failed to open " + resourcePath + " for reading.");
    }

    // Look for IDCL magic
    if (memcmp(memoryMappedFile->memp, "IDCL", 4) != 0)
        throwError(fs::path(resourcePath).filename().string() + " is not a valid .resources file.");

    memPosition += 4;

    // Create out path
    fs::create_directories(outPath, ec);

    if (ec.value() != 0)
        throwError("Failed to create out directory: " + ec.message());

    // Read resource data
    memPosition += 28;

    uint32_t fileCount = memoryMappedFile->readUint32(memPosition);
    memPosition += 8;

    uint32_t dummyCount = memoryMappedFile->readUint32(memPosition);
    memPosition += 24;

    // Get offsets
    uint64_t namesOffset = memoryMappedFile->readUint64(memPosition);
    memPosition += 16;

    uint64_t infoOffset = memoryMappedFile->readUint64(memPosition);
    memPosition += 16;

    uint64_t dummyOffset = memoryMappedFile->readUint64(memPosition) + dummyCount * sizeof(dummyCount);

    memPosition = namesOffset;

    // Get filenames for exporting
    uint64_t nameCount = memoryMappedFile->readUint64(memPosition);
    memPosition += 8;

    std::vector<std::string> names;
    names.reserve(nameCount);

    std::vector<char> nameChars;
    nameChars.reserve(512);

    size_t currentPosition = memPosition;

    for (int i = 0; i < nameCount; i++) {
        memPosition = currentPosition + i * 8;
        uint64_t currentNameOffset = memoryMappedFile->readUint64(memPosition);
        memPosition = namesOffset + nameCount * 8 + currentNameOffset + 8;

        while (*(memoryMappedFile->memp + memPosition) != '\0') {
            const char c = static_cast<char>(*(memoryMappedFile->memp + memPosition));
            nameChars.push_back(c);
            memPosition++;
        }

        names.emplace_back(nameChars.data(), nameChars.size());
        nameChars.clear();
    }

    memPosition = infoOffset;

    // Extract files
    size_t filesExtracted = 0;

    for (int i = 0; i < fileCount; i++) {
        memPosition += 32;

        // Read file info for extracting
        uint64_t nameIdOffset = memoryMappedFile->readUint64(memPosition);
        memPosition += 24;

        uint64_t offset = memoryMappedFile->readUint64(memPosition);
        memPosition += 8;

        uint64_t zSize = memoryMappedFile->readUint64(memPosition);
        memPosition += 8;

        uint64_t size = memoryMappedFile->readUint64(memPosition);
        memPosition += 40;

        uint64_t zipFlags = memoryMappedFile->readUint64(memPosition);
        memPosition += 32;

        nameIdOffset = (nameIdOffset + 1) * 8 + dummyOffset;
        currentPosition = memPosition;
        memPosition = nameIdOffset;

        uint64_t nameId = memoryMappedFile->readUint64(memPosition);
        std::string name = names[nameId];

        // Match filename with regexes
        bool extract = regexesToMatch.empty();

        for (const auto& regex : regexesToMatch) {
            if (std::regex_match(name, regex)) {
                extract = true;
                break;
            }
        }

        if (!extract) {
            memPosition = currentPosition;
            continue;
        }

        for (const auto& regex : regexesNotToMatch) {
            if (std::regex_match(name, regex)) {
                extract = false;
                break;
            }
        }

        if (!extract) {
            memPosition = currentPosition;
            continue;
        }

        // Extract file
        std::cout << "Extracting " << name << "...\n";

        // Create out directory
        auto filePath = fs::path(outPath + name).make_preferred();
        mkpath(filePath, outPath.length());

        if (mkpath(filePath, outPath.length()) != 0)
            throwError("Failed to create " + filePath.parent_path().string() + " path for extraction: " + strerror(errno));

        if (size == 0) {
            // Create empty file and continue
#ifdef _WIN32
            FILE *exportFile = _wfopen(filePath.c_str(), L"wb");
#else
            FILE *exportFile = fopen(filePath.c_str(), "wb");
#endif
            filesExtracted++;
            memPosition = currentPosition;
            continue;
        }

        if (size == zSize) {
            // File is decompressed, extract as-is
#ifdef _WIN32
            MemoryMappedFile *outFile;

            try {
                outFile = new MemoryMappedFile(filePath, size, true, true);
            }
            catch (const std::exception& e) {
                throwError("Failed to extract " + filePath.string() + " for reading.");
            }

            memcpy(outFile->memp, memoryMappedFile->memp + offset, size);
            delete outFile;
#else
            FILE *exportFile = fopen(filePath.c_str(), "wb");

            if (exportFile == nullptr)
                throwError("Failed to open " + filePath.string() + " for writing: " + strerror(errno));

            fwrite(memoryMappedFile->memp + offset, 1, size, exportFile);
            fclose(exportFile);
#endif
        }
        else {
            // File is compressed, decompress with oodle

            // Check oodle flags
            if (zipFlags & 4) {
                offset += 12;
                zSize -= 12;
            }

            // Decompress file
            auto *decBytes = new(std::nothrow) unsigned char[size];

            if (decBytes == nullptr)
                throwError("Failed to allocate memory for extraction.");

            if (OodleLZ_Decompress(memoryMappedFile->memp + offset, static_cast<int32_t>(zSize),
            decBytes, size) != size)
                throwError("Failed to decompress " + name + ".");

            // Write file to disk
#ifdef _WIN32
            MemoryMappedFile *outFile;

            try {
                outFile = new MemoryMappedFile(filePath, size, true, true);
            }
            catch (const std::exception& e) {
                throwError("Failed to open " + filePath.string() + " for writing.");
            }

            memcpy(outFile->memp, decBytes, size);
            delete outFile;
#else
            FILE *exportFile = fopen(filePath.c_str(), "wb");

            if (exportFile == nullptr)
                throwError("Failed to open " + filePath.string() + " for writing: " + strerror(errno));

            fwrite(decBytes, 1, size, exportFile);
            fclose(exportFile);
#endif

            delete[] decBytes;
        }

        filesExtracted++;

        // Seek back to info section
        memPosition = currentPosition;
    }

    delete memoryMappedFile;

    // Exit
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    double totalTime = static_cast<double>(chrono::duration_cast<chrono::microseconds>(end - begin).count());
    double totalTimeSeconds = totalTime / 1000000;

    std::cout.clear();
    std::cout << "\nDone, " << filesExtracted << " files extracted in " << totalTimeSeconds << " seconds." << std::endl;
    pressAnyKey();
}
