#include <filesystem>
#include "mmap.hpp"

// MemoryMappedFile constructor
MemoryMappedFile::MemoryMappedFile(const std::string path)
{
    filePath = path;
    size = std::filesystem::file_size(filePath);

    if (size <= 0) {
        throw std::exception();
    }

#ifdef _WIN32
    // Open the file
    fileHandle = CreateFileA(filePath.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (GetLastError() != ERROR_SUCCESS || fileHandle == INVALID_HANDLE_VALUE) {
        throw std::exception();
    }

    // Map the file to memory
    fileMapping = CreateFileMappingA(fileHandle, nullptr, PAGE_READONLY, *((DWORD*)&size + 1), *(DWORD*)&size, nullptr);

    if (GetLastError() != ERROR_SUCCESS || fileMapping == nullptr) {
        CloseHandle(fileHandle);
        throw std::exception();
    }

    // Get file's memory view
    memp = (unsigned char*)MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);

    if (GetLastError() != ERROR_SUCCESS || memp == nullptr) {
        CloseHandle(fileHandle);
        CloseHandle(fileMapping);
        throw std::exception();
    }
#else
    // Open the file
    fileDescriptor = open(filePath.c_str(), O_RDONLY);

    if (fileDescriptor == -1) {
        throw std::exception();
    }

    // Map the file to memory
    memp = (unsigned char*)mmap(0, size, PROT_READ, MAP_PRIVATE, fileDescriptor, 0);

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

/*
Old:
1.704
1.357
1.616

New:
1.120
1.236
1.162
*/