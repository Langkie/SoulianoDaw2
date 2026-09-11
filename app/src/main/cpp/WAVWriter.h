#pragma once
#include <cstdint>
#include <cstdio>
#include <string>

class WAVWriter {
public:
    WAVWriter() = default;
    ~WAVWriter();

    // Open file for 16-bit PCM
    bool open(const std::string& path, int sampleRate, int channels);
    // Write interleaved float frames [numFrames * channels]
    bool writeFloats(const float* buffer, int numFrames);
    // Close and finalize header
    void close();

private:
    FILE* file = nullptr;
    uint32_t dataBytes = 0;
    int sampleRate = 0;
    int channels = 0;

    void writeHeaderPlaceholder();
    void finalizeHeader();
    static int16_t floatToInt16(float f);
};
