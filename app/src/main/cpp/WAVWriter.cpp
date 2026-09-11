#include "WAVWriter.h"
#include <cstring>
#include <algorithm>

WAVWriter::~WAVWriter() {
    close();
}

bool WAVWriter::open(const std::string& path, int sr, int ch) {
    close();
    file = fopen(path.c_str(), "wb");
    if (!file) return false;
    sampleRate = sr;
    channels = ch;
    dataBytes = 0;
    writeHeaderPlaceholder();
    return true;
}

void WAVWriter::writeHeaderPlaceholder() {
    // RIFF header placeholder (will be rewritten on finalize)
    const unsigned char header[44] = {0};
    fwrite(header, 1, 44, file);
}

int16_t WAVWriter::floatToInt16(float f) {
    f = std::max(-1.0f, std::min(1.0f, f));
    return static_cast<int16_t>(f * 32767.0f);
}

bool WAVWriter::writeFloats(const float* buffer, int numFrames) {
    if (!file) return false;
    int samples = numFrames * channels;
    // Convert to 16-bit interleaved
    for (int i = 0; i < samples; ++i) {
        int16_t s = floatToInt16(buffer[i]);
        fwrite(&s, sizeof(int16_t), 1, file);
        dataBytes += sizeof(int16_t);
    }
    return true;
}

void WAVWriter::finalizeHeader() {
    if (!file) return;
    // WAV header fields
    uint32_t chunkSize = 36 + dataBytes;
    uint16_t audioFormat = 1; // PCM
    uint16_t numChannels = static_cast<uint16_t>(channels);
    uint32_t byteRate = sampleRate * numChannels * 2; // 16-bit
    uint16_t blockAlign = numChannels * 2;
    uint16_t bitsPerSample = 16;

    fseek(file, 0, SEEK_SET);

    // RIFF
    fwrite("RIFF", 1, 4, file);
    fwrite(&chunkSize, sizeof(chunkSize), 1, file);
    fwrite("WAVE", 1, 4, file);

    // fmt subchunk
    fwrite("fmt ", 1, 4, file);
    uint32_t subchunk1Size = 16;
    fwrite(&subchunk1Size, sizeof(subchunk1Size), 1, file);
    fwrite(&audioFormat, sizeof(audioFormat), 1, file);
    fwrite(&numChannels, sizeof(numChannels), 1, file);
    fwrite(&sampleRate, sizeof(sampleRate), 1, file);
    fwrite(&byteRate, sizeof(byteRate), 1, file);
    fwrite(&blockAlign, sizeof(blockAlign), 1, file);
    fwrite(&bitsPerSample, sizeof(bitsPerSample), 1, file);

    // data subchunk
    fwrite("data", 1, 4, file);
    fwrite(&dataBytes, sizeof(dataBytes), 1, file);
}

void WAVWriter::close() {
    if (file) {
        finalizeHeader();
        fclose(file);
        file = nullptr;
    }
    dataBytes = 0;
    sampleRate = 0;
    channels = 0;
}
