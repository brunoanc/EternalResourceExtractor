#include <filesystem>
#include "mmap.hpp"

/**
 * @brief Construct a new MemoryMappedFile object
 * 
 * @param filePath Path to the file to map in memory
 */
MemoryMappedFile::MemoryMappedFile(const std::string path)
{
    filePath = path;
    size = std::filesystem::file_size(filePath);

    if (size <= 0) {
        throw std::exception();
    }

#ifdef _WIN32
    fileHandle = CreateFileA(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (GetLastError() != ERROR_SUCCESS || fileHandle == INVALID_HANDLE_VALUE) {
        throw std::exception();
    }

    fileMapping = CreateFileMappingA(fileHandle, nullptr, PAGE_READWRITE, *((DWORD*)&size + 1), *(DWORD*)&size, nullptr);

    if (GetLastError() != ERROR_SUCCESS || fileMapping == nullptr) {
        CloseHandle(fileHandle);
        throw std::exception();
    }

    memp = (unsigned char*)MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    if (GetLastError() != ERROR_SUCCESS || memp == nullptr) {
        CloseHandle(fileHandle);
        CloseHandle(fileMapping);
        throw std::exception();
    }
#else
    fileDescriptor = open(filePath.c_str(), O_RDWR);

    if (fileDescriptor == -1) {
        throw std::exception();
    }

    memp = (unsigned char*)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);

    if (memp == nullptr) {
        close(fileDescriptor);
        throw std::exception();
    }

    madvise(memp, size, MADV_WILLNEED);
#endif
}

/**
 * @brief Destroy the MemoryMappedFile object
 * 
 */
MemoryMappedFile::~MemoryMappedFile()
{
    unmapFile();
}

/**
 * @brief Unmap the memory mapped file
 * 
 */
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

/**
 * @brief Resize the memory mapped file
 * 
 * @param newSize New size for file
 * @return True on success, false otherwise
 */
bool MemoryMappedFile::resizeFile(const uint64_t newSize)
{
    try {
#ifdef _WIN32
        UnmapViewOfFile(memp);
        CloseHandle(fileMapping);

        fileMapping = CreateFileMappingA(fileHandle, nullptr, PAGE_READWRITE, *((DWORD*)&newSize + 1), *(DWORD*)&newSize, nullptr);

        if (GetLastError() != ERROR_SUCCESS || fileMapping == nullptr) {
            return false;
        }

        memp = (unsigned char*)MapViewOfFile(fileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        if (GetLastError() != ERROR_SUCCESS || memp == nullptr) {
            return false;
        }
#else
        munmap(memp, size);
        std::filesystem::resize_file(filePath, newSize);
        memp = (unsigned char*)mmap(0, newSize, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);

        if (memp == nullptr) {
            return false;
        }

        madvise(memp, newSize, MADV_WILLNEED);
#endif
    }
    catch (...) {
        return false;
    }

    size = newSize;
    return true;
}

int32_t MemoryMappedFile::readInt32(const size_t offset)
{
    if (size - offset < sizeof(uint32_t)) {
        throw std::runtime_error("Failed to read int32 from memory mapped file: Not enough bytes left to read");
    }

    return *(int32_t*)(memp + offset);
}

uint32_t MemoryMappedFile::readUint32(const size_t offset)
{
    return (uint32_t)readInt32(offset);
}

int64_t MemoryMappedFile::readInt64(const size_t offset)
{
    if (size - offset < sizeof(uint64_t)) {
        throw std::runtime_error("Failed to read int64 from memory mapped file: Not enough bytes left to read");
    }

    return *(int64_t*)(memp + offset);
}

uint64_t MemoryMappedFile::readUint64(const size_t offset)
{
    return (uint64_t)readInt64(offset);
}
