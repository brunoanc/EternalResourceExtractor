#ifndef MMAP_HPP
#define MMAP_HPP

#include <string>
#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

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
    uint32_t readUint32(const size_t offset);
    uint64_t readUint64(const size_t offset);
private:
#ifdef _WIN32
    HANDLE fileHandle;
    HANDLE fileMapping;
#else
    int fileDescriptor;
#endif
};

// Read functions - must be in header to avoid unresolved symbol errors
inline uint32_t MemoryMappedFile::readUint32(const size_t offset)
{
    return *(uint32_t*)(memp + offset);
}

inline uint64_t MemoryMappedFile::readUint64(const size_t offset)
{
    return *(uint64_t*)(memp + offset);
}

#endif
