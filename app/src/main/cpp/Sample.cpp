#include "Sample.h"
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>

static uint32_t read_u32(FILE *f) {
    uint32_t v = 0;
    fread(&v, sizeof(v), 1, f);
    return v;
}

static uint16_t read_u16(FILE *f) {
    uint16_t v = 0;
    fread(&v, sizeof(v), 1, f);
    return v;
}

bool loadWavFile(const std::string &path, Sample &outSample) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) {
        std::cerr << "Failed to open WAV: " << path << std::endl;
        return false;
    }

    char riff[4];
    if (fread(riff, 1, 4, f) != 4) { fclose(f); return false; }
    if (std::strncmp(riff, "RIFF", 4) != 0) {
        fclose(f);
        std::cerr << "Not a RIFF file" << std::endl;
        return false;
    }

    // skip file size and WAVE
    fseek(f, 8, SEEK_SET);
    // Search chunks for fmt and data
    uint32_t chunkId;
    uint32_t chunkSize;
    bool fmtFound = false;
    bool dataFound = false;
    int audioFormat = 0;
    int channels = 0;
    int sampleRate = 0;
    uint32_t dataBytes = 0;
    long dataPos = 0;

    while (fread(&chunkId, sizeof(chunkId), 1, f) == 1) {
        if (fread(&chunkSize, sizeof(chunkSize), 1, f) != 1) break;
        if (chunkId == 0x20746d66) { // 'fmt '
            fmtFound = true;
            audioFormat = read_u16(f);
            channels = read_u16(f);
            sampleRate = read_u32(f);
            fseek(f, 6, SEEK_CUR); // byteRate + blockAlign
            uint16_t bitsPerSample = read_u16(f);
            // skip any extra bytes in fmt chunk
            if (chunkSize > 16) fseek(f, chunkSize - 16, SEEK_CUR);
        } else if (chunkId == 0x61746164) { // 'data'
            dataFound = true;
            dataBytes = chunkSize;
            dataPos = ftell(f);
            fseek(f, chunkSize, SEEK_CUR);
        } else {
            // skip this chunk
            fseek(f, chunkSize, SEEK_CUR);
        }
    }

    if (!fmtFound || !dataFound) {
        fclose(f);
        std::cerr << "Missing fmt or data chunk" << std::endl;
        return false;
    }

    // Read data
    if (freopen(path.c_str(), "rb", f) == nullptr) { fclose(f); return false; }
    fseek(f, dataPos, SEEK_SET);

    int16_t sample = 0;
    uint32_t totalSamples = dataBytes / sizeof(int16_t);
    uint32_t frames = totalSamples / channels;

    outSample.sampleRate = sampleRate;
    outSample.channels = channels;
    outSample.data.resize(totalSamples);

    for (uint32_t i = 0; i < totalSamples; ++i) {
        if (fread(&sample, sizeof(int16_t), 1, f) != 1) {
            fclose(f);
            std::cerr << "Unexpected EOF while reading samples" << std::endl;
            return false;
        }
        // convert int16 to float
        outSample.data[i] = static_cast<float>(sample) / 32767.0f;
    }

    fclose(f);
    return true;
}
