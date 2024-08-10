#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

#include "mmap.hpp"

// MemoryMappedFile constructor
MemoryMappedFile::MemoryMappedFile(const fs::path &path, size_t size, bool create, bool sequential)
{
    filePath = path.string();

    if (size == -1)
        size = fs::file_size(path);

    if (size <= 0)
        throw std::exception();

#ifdef _WIN32
    // Open the file
    fileHandle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, (create ? CREATE_ALWAYS : OPEN_EXISTING), (sequential ? FILE_FLAG_SEQUENTIAL_SCAN : FILE_ATTRIBUTE_NORMAL), nullptr);

    if ((GetLastError() != ERROR_SUCCESS && GetLastError() != 183) || fileHandle == INVALID_HANDLE_VALUE)
        throw std::exception();

    // Map the file to memory
    fileMapping = CreateFileMappingW(fileHandle, nullptr, PAGE_READWRITE, *((DWORD*)&size + 1), *(DWORD*)&size, nullptr);

    if (GetLastError() != ERROR_SUCCESS || fileMapping == nullptr) {
        CloseHandle(fileHandle);
        throw std::exception();
    }

    // Get file's memory view
    memp = (unsigned char*)MapViewOfFile(fileMapping, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

    if (GetLastError() != ERROR_SUCCESS || memp == nullptr) {
        CloseHandle(fileHandle);
        CloseHandle(fileMapping);
        throw std::exception();
    }
#else
    // Open the file
    fileDescriptor = open(filePath.c_str(), O_RDWR);

    if (fileDescriptor == -1)
        throw std::exception();

    // Map the file to memory
    memp = reinterpret_cast<unsigned char*>(mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0));

    if (memp == nullptr) {
        close(fileDescriptor);
        throw std::exception();
    }

    madvise(memp, size, MADV_WILLNEED);
#endif
}

// MemoryMappedFile destructor
MemoryMappedFile::~MemoryMappedFile()
{
    unmapFile();
}

// Unmap the file from memory
void MemoryMappedFile::unmapFile()
{
    if (memp == nullptr)
        return;

#ifdef _WIN32
    UnmapViewOfFile(memp);
    CloseHandle(fileMapping);
    CloseHandle(fileHandle);
#else
    munmap(memp, size);
    close(fileDescriptor);
#endif

    size = 0;
    memp = nullptr;
}

// Read functions
uint32_t MemoryMappedFile::readUint32LE(size_t &offset)
{
    uint32_t result = *reinterpret_cast<uint32_t*>(memp + offset);
    offset += 4;
    return result;
}

uint64_t MemoryMappedFile::readUint64LE(size_t &offset)
{
    uint64_t result = *reinterpret_cast<uint64_t*>(memp + offset);
    offset += 8;
    return result;
}

uint32_t MemoryMappedFile::readUint32BE(size_t &offset)
{
    uint32_t result = *reinterpret_cast<uint32_t*>(memp + offset);
    std::reverse(reinterpret_cast<unsigned char*>(&result), reinterpret_cast<unsigned char*>(&result) + 4);
    offset += 4;
    return result;
}

uint64_t MemoryMappedFile::readUint64BE(size_t &offset)
{
    uint64_t result = *reinterpret_cast<uint64_t*>(memp + offset);
    std::reverse(reinterpret_cast<unsigned char*>(&result), reinterpret_cast<unsigned char*>(&result) + 8);
    offset += 8;
    return result;
}
