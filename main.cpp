#include <iostream>
#include <cstring>
#include <chrono>
#include <array>
#include "extract.hpp"
#include "utils.hpp"
#include "argh/argh.h"

namespace chrono = std::chrono;

int main(int argc, char **argv)
{
    // Disable sync with stdio
    std::ios::sync_with_stdio(false);

    // Buffer stdout
    std::array<char, 8192> buffer;
    std::cout.rdbuf()->pubsetbuf(buffer.data(), buffer.size());

    std::cout << "EternalResourceExtractor v3.3.0 by powerball253\n\n";

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

    // Get regexes to match/not match
    std::vector<std::regex> regexesToMatch;
    std::vector<std::regex> regexesNotToMatch;
    compileRegexes(regexesToMatch, regexesNotToMatch, cmdl.params());

    // Time program
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();

    // Open the resource file
    MemoryMappedFile *memoryMappedFile;

    try {
        memoryMappedFile = new MemoryMappedFile(resourcePath);
    }
    catch (const std::exception &e) {
        throwError("Failed to open " + resourcePath + " for reading.");
    }
    
    // Create out path
    fs::create_directories(outPath, ec);

    if (ec.value() != 0)
        throwError("Failed to create out directory: " + ec.message());

    size_t filesExtracted = 0;

    // Identify file using magic and extract
    if (memcmp(memoryMappedFile->memp, "IDCL", 4) == 0)
        filesExtracted = extractResource(memoryMappedFile, outPath, regexesToMatch, regexesNotToMatch);
    else if (*reinterpret_cast<uint32_t*>(memoryMappedFile->memp) == 131121354)
        filesExtracted = extractWad7(memoryMappedFile, outPath, regexesToMatch, regexesNotToMatch);
    else
        throwError(fs::path(resourcePath).filename().string() + " is not a valid .resources or .wad7 file.");

    delete memoryMappedFile;

    // Exit
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    double totalTime = static_cast<double>(chrono::duration_cast<chrono::microseconds>(end - begin).count());
    double totalTimeSeconds = totalTime / 1000000;

    std::cout.clear();
    std::cout << "\nDone, " << filesExtracted << " files extracted in " << totalTimeSeconds << " seconds." << std::endl;
    pressAnyKey();
}
