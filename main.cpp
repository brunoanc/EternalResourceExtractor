#include <iostream>
#include <string>
#include <vector>
#include <cerrno>
#include <cstring>
#include <chrono>
#include "utils.hpp"

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
    FILE *resource = openFile(resourcePath, "rb");

    if (resource == nullptr)
        throwError("Failed to open " + resourcePath + " for reading: " + strerror(errno));

    if (!oodleInit(&OodLZ_Decompress))
        throwError("Failed to init oodle for decompressing.");

    // Look for IDCL magic
    char signature[4];
    fread(signature, 1, sizeof(signature), resource);

    if (std::memcmp(signature, "IDCL", 4) != 0)
        throwError(fs::path(resourcePath).filename().string() + " is not a valid .resources file.");

    // Create out path
    ec = mkpath(outPath);

    if (ec.value() != 0)
        throwError("Failed to create out directory: " + ec.message());

    // Read resource data
    fseek(resource, 28, SEEK_CUR);

    uint32_t fileCount;
    fread(&fileCount, sizeof(fileCount), 1, resource);

    fseek(resource, 4, SEEK_CUR);

    uint32_t dummyCount;
    fread(&dummyCount, sizeof(dummyCount), 1, resource);

    fseek(resource, 20, SEEK_CUR);

    // Get offsets
    uint64_t namesOffset;
    fread(&namesOffset, sizeof(namesOffset), 1, resource);

    fseek(resource, 8, SEEK_CUR);

    uint64_t infoOffset;
    fread(&infoOffset, sizeof(infoOffset), 1, resource);

    fseek(resource, 8, SEEK_CUR);

    uint64_t dummyOffset;
    fread(&dummyOffset, sizeof(dummyOffset), 1, resource);
    dummyOffset += dummyCount * sizeof(uint32_t);

    uint64_t dataOffset;
    fread(&dataOffset, sizeof(dataOffset), 1, resource);

    fseek(resource, static_cast<long>(namesOffset), SEEK_SET);

    // Get filenames for exporting
    uint64_t nameCount;
    fread(&nameCount, sizeof(nameCount), 1, resource);

    std::vector<std::string> names;
    names.reserve(nameCount);

    size_t currentPosition = ftell(resource);

    for (int i = 0; i < nameCount; i++) {
        fseek(resource, static_cast<long>(currentPosition) + i * 8, SEEK_SET);

        uint64_t currentNameOffset;
        fread(&currentNameOffset, sizeof(currentNameOffset), 1, resource);

        fseek(resource, static_cast<long>(namesOffset + nameCount * 8 + currentNameOffset + 8), SEEK_SET);

        char *name = nullptr;
        size_t len = 0;
        len = fgetdelim(&name, &len, '\0', resource);

        if (len == -1)
            throwError("Failed to read file names from resource.");

        names.emplace_back(name, len);
        free(name);
    }

    fseek(resource, static_cast<long>(infoOffset), SEEK_SET);

    // Extract files
    for (int i = 0; i < fileCount; i++) {
        fseek(resource, 24, SEEK_CUR);

        // Read file info for extracting
        uint64_t typeIdOffset;
        fread(&typeIdOffset, sizeof(typeIdOffset), 1, resource);

        uint64_t nameIdOffset;
        fread(&nameIdOffset, sizeof(nameIdOffset), 1, resource);

        fseek(resource, 16, SEEK_CUR);

        uint64_t offset;
        fread(&offset, sizeof(offset), 1, resource);

        uint64_t zSize;
        fread(&zSize, sizeof(zSize), 1, resource);

        uint64_t size;
        fread(&size, sizeof(size), 1, resource);

        fseek(resource, 32, SEEK_CUR);

        uint64_t zipFlags;
        fread(&zipFlags, sizeof(zipFlags), 1, resource);

        fseek(resource, 24, SEEK_CUR);

        typeIdOffset = typeIdOffset * 8 + dummyOffset;
        nameIdOffset = (nameIdOffset + 1) * 8 + dummyOffset;

        currentPosition = ftell(resource);

        fseek(resource, static_cast<long>(typeIdOffset), SEEK_SET);

        uint64_t typeId;
        fread(&typeId, sizeof(typeId), 1, resource);

        fseek(resource, static_cast<long>(nameIdOffset), SEEK_SET);

        uint64_t nameId;
        fread(&nameId, sizeof(nameId), 1, resource);

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
            fseek(resource, static_cast<long>(offset), SEEK_SET);

            // Read file from resource
            auto fileBytes = new (std::nothrow) unsigned char[zSize];

            if (fileBytes == nullptr)
                throwError("Failed to allocate memory for extraction.");

            fread(fileBytes, 1, zSize, resource);

            // Write file to disk
            FILE *exportFile = openFile(path, "wb");

            if (exportFile == nullptr)
                throwError("Failed to open " + path + " for writing: " + strerror(errno));

            fwrite(fileBytes, 1, zSize, exportFile);
            fclose(exportFile);

            delete[] fileBytes;
        }
        else {
            // File is compressed, decompress with oodle

            // Check oodle flags
            if (zipFlags & 4) {
                offset += 12;
                zSize -= 12;
            }

            fseek(resource, static_cast<long>(offset), SEEK_SET);

            // Read file from resource
            auto *encBytes = new (std::nothrow) unsigned char[zSize];

            if (encBytes == nullptr)
                throwError("Failed to allocate memory for extraction.");

            fread(encBytes, 1, zSize, resource);

            // Decompress file
            auto *decBytes = new (std::nothrow) unsigned char[size];

            if (decBytes == nullptr)
                throwError("Failed to allocate memory for extraction.");

            if (OodLZ_Decompress(encBytes, static_cast<int32_t>(zSize), decBytes, size, 0, 0, 0, nullptr, 0, nullptr, nullptr, nullptr, 0, 0) != size)
                throwError("Failed to decompress " + name + ".");

            delete[] encBytes;

            // Write file to disk
            FILE *exportFile = openFile(path, "wb");

            if (exportFile == nullptr)
                throwError("Failed to open " + path + " for writing: " + strerror(errno));

            fwrite(decBytes, 1, size, exportFile);
            fclose(exportFile);

            delete[] decBytes;
        }

        // Seek back to info section
        fseek(resource, static_cast<long>(currentPosition), SEEK_SET);
    }

    fclose(resource);

    // Exit
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    double totalTime = chrono::duration_cast<chrono::microseconds>(end - begin).count() / 1000000.0;
    std::cout << "\nDone, " << fileCount << " files extracted in " << totalTime << " seconds." << std::endl;
    pressAnyKey();
}
