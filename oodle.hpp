#ifndef OODLE_HPP
#define OODLE_HPP

#include <iostream>

#ifdef _WIN32
// Use __stdcall linkage on Windows
#define LINKAGE __stdcall
#else
#define LINKAGE
#endif

extern "C"
{
    // OodleDecompressionCalback typedef
    typedef int (LINKAGE OodleDecompressCallback)(void *userdata, const uint8_t *rawBuf, intptr_t rawLen,
        const uint8_t *compBuf, intptr_t compBufferSize, intptr_t rawDone, intptr_t compUsed);

    // Oodle decompress function
    intptr_t LINKAGE OodleLZ_Decompress(const void *compBuf, intptr_t compBufSize, void *rawBuf, intptr_t rawLen,
        int fuzzSafe = 1, int checkCRC = 0, int verbosity = 0, void *decBufBase = nullptr, intptr_t decBufSize = 0,
        OodleDecompressCallback *fpCallback = nullptr, void *callbackUserData = nullptr, void *decoderMemory = nullptr,
        intptr_t decoderMemorySize = 0, int threadPhase = 3);
}

#endif
