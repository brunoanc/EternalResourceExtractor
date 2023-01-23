#ifndef OOZ_HPP
#define OOZ_HPP

#include <cstdint>
#include <cstddef>

extern "C" {
    int Kraken_Compress(uint8_t* src, size_t src_len, uint8_t* dst, int level);
    int Kraken_Decompress(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len);
};

#endif
