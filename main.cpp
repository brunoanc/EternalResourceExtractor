#include <iostream>
#include <string>
#include <vector>
#include <cerrno>
#include <cstring>
#include <chrono>
#include "utils.hpp"
#include "mmap/mmap.hpp"

namespace chrono = std::chrono;

static OodLZ_DecompressFunc *OodLZ_Decompress = nullptr;

int main(int argc, char **argv)
{
    // Time program
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();

    // Disable sync with stdio
    std::ios::sync_with_stdio(false);

    // Buffer stdout
    char buffer[8192];
    std::cout.rdbuf()->pubsetbuf(buffer, sizeof(buffer));

    std::cout << "EternalResourceExtractor v1.0 by PowerBall253\n\n";

    // Print help
    if (argc > 1 && !strcmp(argv[1], "--help")) {
        std::cout << "Usage:\n";
        std::cout << "%s [path to .resources file] [out path]\n";
        std::cout.flush();
        return 1;
    }

    // Get resource & out path
    std::string resourcePath;
    std::string outPath;
    std::error_code ec;

    switch (argc) {
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
            resourcePath = std::string(argv[1]);

            std::cout << "Input the path to the out directory: ";
            std::cout.flush();
            std::getline(std::cin, outPath);

            std::cout << '\n';
            break;
        default:
            resourcePath = std::string(argv[1]);
            outPath = std::string(argv[2]);
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

    // Open the resource file
    MemoryMappedFile *memoryMappedFile;
    size_t memPosition = 0;

    try {
        memoryMappedFile = new MemoryMappedFile(resourcePath);
    }
    catch (std::exception &e) {
        throwError("Failed to open " + resourcePath + " for reading.");
    }

    if (!oodleInit(&OodLZ_Decompress))
        throwError("Failed to init oodle for decompressing.");

    // Look for IDCL magic
    if (std::memcmp(memoryMappedFile->memp, "IDCL", 4) != 0)
        throwError(fs::path(resourcePath).filename().string() + " is not a valid .resources file.");

    memPosition += 4;

    // Create out path
    ec = mkpath(outPath);

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
    memPosition += 8;

    uint64_t dataOffset = memoryMappedFile->readUint64(memPosition);

    memPosition = namesOffset;

    // Get filenames for exporting
    uint64_t nameCount = memoryMappedFile->readUint64(memPosition);
    memPosition += 8;

    std::vector<std::string> names;
    names.reserve(nameCount);

    std::vector<char> name;
    name.reserve(256);

    size_t currentPosition = memPosition;

    for (int i = 0; i < nameCount; i++) {
        memPosition = currentPosition + i * 8;

        uint64_t currentNameOffset = memoryMappedFile->readUint64(memPosition);

        memPosition = namesOffset + nameCount * 8 + currentNameOffset + 8;

        do {
            name.push_back(*(memoryMappedFile->memp + memPosition));
            memPosition++;
        } while (*(memoryMappedFile->memp + memPosition) != '\0');

        names.emplace_back(name.data(), name.size());
        name.clear();
    }

    memPosition = infoOffset;

    // Extract files
    for (int i = 0; i < fileCount; i++) {
        memPosition += 24;

        // Read file info for extracting
        uint64_t typeIdOffset = memoryMappedFile->readUint64(memPosition);
        memPosition += 8;

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

        typeIdOffset = typeIdOffset * 8 + dummyOffset;
        nameIdOffset = (nameIdOffset + 1) * 8 + dummyOffset;
        currentPosition = memPosition;
        memPosition = typeIdOffset;

        uint64_t typeId = memoryMappedFile->readUint64(memPosition);

        memPosition = nameIdOffset;

        uint64_t nameId = memoryMappedFile->readUint64(memPosition);
        memPosition += 8;

        auto name = names[nameId];

        // Extract file
        std::cout << "Extracting " << name << "...\n";

        // Create out directory
        std::string path = outPath + name;
        path = fs::path(path).make_preferred().string();
        ec = mkpath(path);

        if (ec.value() != 0)
            throwError("Failed to create path for extraction: " + ec.message());

        if (size == zSize) {
            // File is decompressed, extract as-is
            FILE *exportFile = openFile(path, "wb");

            if (exportFile == nullptr)
                throwError("Failed to open " + path + " for writing: " + strerror(errno));

            fwrite(memoryMappedFile->memp + offset, 1, size, exportFile);
            fclose(exportFile);
        }
        else {
            // File is compressed, decompress with oodle

            // Check oodle flags
            if (zipFlags & 4) {
                offset += 12;
                zSize -= 12;
            }

            // Decompress file
            auto *decBytes = new (std::nothrow) unsigned char[size];

            if (decBytes == nullptr)
                throwError("Failed to allocate memory for extraction.");

            if (OodLZ_Decompress(memoryMappedFile->memp + offset, static_cast<int32_t>(zSize),
            decBytes, size, 0, 0, 0, nullptr, 0, nullptr, nullptr, nullptr, 0, 0) != size)
                throwError("Failed to decompress " + name + ".");

            // Write file to disk
            FILE *exportFile = openFile(path, "wb");

            if (exportFile == nullptr)
                throwError("Failed to open " + path + " for writing: " + strerror(errno));

            fwrite(decBytes, 1, size, exportFile);
            fclose(exportFile);

            delete[] decBytes;
        }

        // Seek back to info section
        memPosition = currentPosition;
    }

    delete memoryMappedFile;

    // Exit
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    double totalTime = chrono::duration_cast<chrono::microseconds>(end - begin).count() / 1000000.0;
    std::cout << "\nDone, " << fileCount << " files extracted in " << totalTime << " seconds." << std::endl;
    pressAnyKey();
}
