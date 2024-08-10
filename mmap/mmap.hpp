#ifndef MMAP_HPP
#define MMAP_HPP

#include <filesystem>

namespace fs = std::filesystem;

// Cross platform memory mapped file
class MemoryMappedFile {
public:
    std::string filePath;
    unsigned char *memp;
    uint64_t size = 0;

    MemoryMappedFile(const fs::path &path, size_t size = -1, bool create = false, bool sequential = false);
    ~MemoryMappedFile();

    void unmapFile();
    uint32_t readUint32LE(size_t &offset);
    uint64_t readUint64LE(size_t &offset);
    uint32_t readUint32BE(size_t &offset);
    uint64_t readUint64BE(size_t &offset);
private:
#ifdef _WIN32
    void *fileHandle;
    void *fileMapping;
#else
    int fileDescriptor;
#endif
};

#endif
