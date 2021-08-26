#ifndef UTILS_H
#define UTILS_H

#include <filesystem>

namespace fs = std::filesystem;

// Oodle typedef
typedef int32_t OodLZ_DecompressFunc(uint8_t *src_buf, int32_t src_len, uint8_t *dst, size_t dst_size,
    int32_t fuzz, int32_t crc, int32_t verbose,
    uint8_t *dst_base, size_t e, void *cb, void *cb_ctx, void *scratch, size_t scratch_size, int32_t threadPhase);

void pressAnyKey();
void throwError(const std::string& error);
std::error_code mkpath(fs::path path);
std::string formatPath(std::string path);
FILE *openFile(const fs::path &path, const char *mode);
bool oodleInit(OodLZ_DecompressFunc **OodLZ_Decompress);

#endif
