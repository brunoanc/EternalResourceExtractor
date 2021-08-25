/*
* This file is part of EternalModLoaderCpp (https://github.com/PowerBall253/EternalModLoaderCpp).
* Copyright (C) 2021 PowerBall253
*
* EternalModLoaderCpp is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* EternalModLoaderCpp is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with EternalModLoaderCpp. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MMAP_HPP
#define MMAP_HPP

#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#endif

/**
 * @brief Cross platform memory mapped file class
 * 
 */
class MemoryMappedFile {
public:
    std::string filePath;
    unsigned char *memp;
    uint64_t size = 0;

    MemoryMappedFile(const std::string filePath);
    MemoryMappedFile() {}
    ~MemoryMappedFile();

    void unmapFile();
    bool resizeFile(const uint64_t newSize);
    int32_t readInt32(const size_t offset);
    uint32_t readUint32(const size_t offset);
    int64_t readInt64(const size_t offset);
    uint64_t readUint64(const size_t offset);
private:
#ifdef _WIN32
    HANDLE fileHandle;
    HANDLE fileMapping;
#else
    int fileDescriptor;
#endif
};

#endif