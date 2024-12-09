#include <filesystem>
#include <iostream>
#include <regex>
#include <cstring>
#include "utils.hpp"
#include "ooz.hpp"
#include "mmap/mmap.hpp"

// Extract file from memory
void extractFile(const MemoryMappedFile *memoryMappedFile, const std::string &name, const std::string &outPath, size_t offset, size_t size, size_t zSize, size_t compressionMode)
{
    std::cout << "Extracting " << name << "...\n";

    // Create out directory
    auto filePath = fs::path(outPath + name).make_preferred();
    mkpath(filePath, outPath.length());

    if (mkpath(filePath, outPath.length()) != 0)
        throwError("Failed to create " + filePath.parent_path().string() + " path for extraction: " + strerror(errno));

    if (fs::is_directory(filePath)) {
        filePath += " (1)";
    }

    if (size == 0) {
        // Create empty file and continue
#ifdef _WIN32
        FILE *exportFile = _wfopen(filePath.c_str(), L"wb");
#else
        FILE *exportFile = fopen(filePath.c_str(), "wb");
#endif
        fclose(exportFile);
    }
    else if (size == zSize) {
        // File is decompressed, extract as-is
#ifdef _WIN32
        MemoryMappedFile *outFile;

        try {
            outFile = new MemoryMappedFile(filePath, size, true, true);
        }
        catch (const std::exception &e) {
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
        // File is kraken-compressed, decompress with ooz

        // Check oodle flags
        if ((compressionMode & 4) != 0) {
            offset += 12;
            zSize -= 12;
        }

        // Decompress file
        auto *decBytes = new(std::nothrow) unsigned char[size + SAFE_SPACE];

        if (decBytes == nullptr)
            throwError("Failed to allocate memory for extraction.");

        if (Kraken_Decompress(memoryMappedFile->memp + offset, static_cast<int32_t>(zSize),
        decBytes, size) != size)
            throwError("Failed to decompress " + name + ".");

        // Write file to disk
#ifdef _WIN32
        MemoryMappedFile *outFile;

        try {
            outFile = new MemoryMappedFile(filePath, size, true, true);
        }
        catch (const std::exception &e) {
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
}

// Check whether we should extract the file based on the include/exclude regexes
bool shouldExtractFile(const std::string &name, const std::vector<std::regex> &regexesToMatch, const std::vector<std::regex> &regexesNotToMatch)
{
    bool extract = regexesToMatch.empty();

    for (const auto &regex : regexesToMatch) {
        if (std::regex_match(name, regex)) {
            extract = true;
            break;
        }
    }

    if (!extract) {
        return false;
    }

    for (const auto &regex : regexesNotToMatch) {
        if (std::regex_match(name, regex)) {
            extract = false;
            break;
        }
    }
    
    return extract;
}

// Extract all files from resources file
size_t extractResource(MemoryMappedFile *memoryMappedFile, const std::string &outPath, const std::vector<std::regex> &regexesToMatch, const std::vector<std::regex> &regexesNotToMatch)
{
    // Read resource data
    size_t memPosition = 4;

    uint32_t version = memoryMappedFile->readUint32LE(memPosition);

    if (version >= 0xD) {
        memPosition = 36;
    }
    else {
        memPosition = 32;
    }

    uint32_t fileCount = memoryMappedFile->readUint32LE(memPosition);
    memPosition += 4;

    uint32_t dummyCount = memoryMappedFile->readUint32LE(memPosition);
    memPosition += 20;

    // Get offsets
    uint64_t namesOffset = memoryMappedFile->readUint64LE(memPosition);
    memPosition += 8;

    uint64_t infoOffset = memoryMappedFile->readUint64LE(memPosition);
    memPosition += 8;

    uint64_t dummyOffset = memoryMappedFile->readUint64LE(memPosition) + dummyCount * sizeof(dummyCount);

    memPosition = namesOffset;

    // Get filenames for exporting
    uint64_t nameCount = memoryMappedFile->readUint64LE(memPosition);

    std::vector<std::string> names;
    names.reserve(nameCount);

    size_t currentPosition = memPosition;

    for (int i = 0; i < nameCount; i++) {
        memPosition = currentPosition + static_cast<size_t>(i) * 8;
        uint64_t currentNameOffset = memoryMappedFile->readUint64LE(memPosition);

        memPosition = namesOffset + nameCount * 8 + currentNameOffset + 8;
        std::string name(reinterpret_cast<char*>(memoryMappedFile->memp) + memPosition);

        memPosition += name.length();
        names.push_back(name);
    }

    memPosition = infoOffset;

    // Extract files
    size_t filesExtracted = 0;

    for (int i = 0; i < fileCount; i++) {
        memPosition += 32;

        // Read file info for extracting
        uint64_t nameIdOffset = memoryMappedFile->readUint64LE(memPosition);
        memPosition += 16;

        uint64_t offset = memoryMappedFile->readUint64LE(memPosition);
        uint64_t zSize = memoryMappedFile->readUint64LE(memPosition);
        uint64_t size = memoryMappedFile->readUint64LE(memPosition);

        memPosition += 32;

        uint64_t zipFlags = memoryMappedFile->readUint64LE(memPosition);
        memPosition += 24;

        nameIdOffset = (nameIdOffset + 1) * 8 + dummyOffset;
        currentPosition = memPosition;
        memPosition = nameIdOffset;

        uint64_t nameId = memoryMappedFile->readUint64LE(memPosition);
        std::string name = names[nameId];

        // Match filename with regexes
        if (!shouldExtractFile(name, regexesToMatch, regexesNotToMatch)) {
            memPosition = currentPosition;
            continue;
        }

        // Extract file
        extractFile(memoryMappedFile, name, outPath, offset, size, zSize, zipFlags);
        filesExtracted++;

        // Seek back to info section
        memPosition = currentPosition;
    }
    
    return filesExtracted;
}

// Extract all files from WAD7 file
size_t extractWad7(MemoryMappedFile *memoryMappedFile, const std::string &outPath, const std::vector<std::regex> &regexesToMatch, const std::vector<std::regex> &regexesNotToMatch)
{
    size_t memPosition = 19;

    // Get index position and size
    uint64_t indexStart = memoryMappedFile->readUint64BE(memPosition);
    uint64_t indexSize = memoryMappedFile->readUint64BE(memPosition);

    memPosition = indexStart;

    // Get entry count
    uint32_t entryCount = memoryMappedFile->readUint32BE(memPosition);

    // Extract files
    size_t filesExtracted = 0;

    for (int i = 0; i < entryCount; i++) {
        // Get entry name
        uint32_t nameSize = memoryMappedFile->readUint32LE(memPosition);

        std::string name(reinterpret_cast<char*>(memoryMappedFile->memp) + memPosition, nameSize);
        memPosition += nameSize;
        
        // Match filename with regexes
        if (!shouldExtractFile(name, regexesToMatch, regexesNotToMatch)) {
            memPosition += 32;
            continue;
        }

        // Get data offset
        uint64_t offset = memoryMappedFile->readUint64BE(memPosition);

        // Get uncompressed size
        uint32_t size = memoryMappedFile->readUint32BE(memPosition);

        // Get compressed size
        uint32_t zSize = memoryMappedFile->readUint32BE(memPosition);
        
        // Get compression mode
        uint32_t compressionMode = memoryMappedFile->readUint32BE(memPosition);

        // Extract file
        extractFile(memoryMappedFile, name, outPath, offset, size, zSize, compressionMode);
        filesExtracted++;
        memPosition += 12;
    }
    
    return filesExtracted;
}
