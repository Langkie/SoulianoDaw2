#include "WavFile.h"
#include <iostream>
#include <cstring>
#include <algorithm>

WavFile::WavFile() = default;

WavFile::~WavFile() = default;

bool WavFile::load(const std::string& filePath) {
    FILE* file = fopen(filePath.c_str(), "rb");
    if (!file) {
        std::cerr << "Failed to open WAV file: " << filePath << std::endl;
        return false;
    }

    if (!readWavHeader(file)) {
        fclose(file);
        return false;
    }

    // Read audio data
    std::vector<uint8_t> rawData(header.dataSize);
    size_t bytesRead = fread(rawData.data(), 1, header.dataSize, file);
    fclose(file);

    if (bytesRead != header.dataSize) {
        std::cerr << "Failed to read complete WAV data. Expected: " << header.dataSize 
                  << ", Read: " << bytesRead << std::endl;
        return false;
    }

    convertToFloat(rawData);
    std::cerr << "Loaded WAV: " << header.numChannels << " channels, " 
              << header.sampleRate << " Hz, " << numSamples << " samples" << std::endl;

    return true;
}

bool WavFile::readWavHeader(FILE* file) {
    char buffer[4];

    // Read RIFF header
    if (fread(buffer, 1, 4, file) != 4 || std::string(buffer, 4) != "RIFF") {
        std::cerr << "Invalid WAV file: Missing RIFF header" << std::endl;
        return false;
    }

    // Skip RIFF size
    uint32_t riffSize;
    if (fread(&riffSize, 4, 1, file) != 1) return false;

    // Read WAVE format
    if (fread(buffer, 1, 4, file) != 4 || std::string(buffer, 4) != "WAVE") {
        std::cerr << "Invalid WAV file: Missing WAVE format" << std::endl;
        return false;
    }

    // Find fmt chunk
    bool foundFmt = false;
    while (fread(buffer, 1, 4, file) == 4) {
        uint32_t chunkSize;
        if (fread(&chunkSize, 4, 1, file) != 1) return false;

        if (std::string(buffer, 4) == "fmt ") {
            uint16_t audioFormat;
            if (fread(&audioFormat, 2, 1, file) != 1) return false;
            if (fread(&header.numChannels, 2, 1, file) != 1) return false;
            if (fread(&header.sampleRate, 4, 1, file) != 1) return false;
            if (fread(&header.byteRate, 4, 1, file) != 1) return false;
            if (fread(&header.blockAlign, 2, 1, file) != 1) return false;
            if (fread(&header.bitsPerSample, 2, 1, file) != 1) return false;

            if (audioFormat != 1) {
                std::cerr << "Unsupported audio format: " << audioFormat << " (only PCM supported)" << std::endl;
                return false;
            }

            foundFmt = true;

            // Skip remaining fmt chunk data
            long remaining = chunkSize - 16;
            if (remaining > 0) {
                fseek(file, remaining, SEEK_CUR);
            }
        } else if (std::string(buffer, 4) == "data") {
            if (!foundFmt) {
                std::cerr << "WAV file has data chunk before fmt chunk" << std::endl;
                return false;
            }
            header.dataSize = chunkSize;
            return true;
        } else {
            // Skip unknown chunk
            fseek(file, chunkSize, SEEK_CUR);
        }
    }

    std::cerr << "WAV file missing data chunk" << std::endl;
    return false;
}

void WavFile::convertToFloat(const std::vector<uint8_t>& rawData) {
    audioData.clear();
    
    uint32_t totalSamples = header.dataSize / header.blockAlign;
    numSamples = totalSamples / header.numChannels;
    audioData.resize(totalSamples);

    const uint8_t* src = rawData.data();
    float* dst = audioData.data();

    if (header.bitsPerSample == 16) {
        const int16_t* src16 = reinterpret_cast<const int16_t*>(src);
        for (uint32_t i = 0; i < totalSamples; ++i) {
            dst[i] = static_cast<float>(src16[i]) / 32768.0f;
        }
    } else if (header.bitsPerSample == 24) {
        for (uint32_t i = 0; i < totalSamples; ++i) {
            int32_t sample = (src[0] | (src[1] << 8) | (src[2] << 16));
            if (sample & 0x800000) sample |= 0xFF000000;
            dst[i] = static_cast<float>(sample) / 8388608.0f;
            src += 3;
        }
    } else if (header.bitsPerSample == 32) {
        if (header.byteRate == header.sampleRate * header.numChannels * 4) {
            // PCM 32-bit
            const int32_t* src32 = reinterpret_cast<const int32_t*>(src);
            for (uint32_t i = 0; i < totalSamples; ++i) {
                dst[i] = static_cast<float>(src32[i]) / 2147483648.0f;
            }
        } else {
            // IEEE 32-bit float
            const float* srcFloat = reinterpret_cast<const float*>(src);
            std::copy(srcFloat, srcFloat + totalSamples, dst);
        }
    } else if (header.bitsPerSample == 8) {
        for (uint32_t i = 0; i < totalSamples; ++i) {
            dst[i] = (static_cast<float>(src[i]) - 128.0f) / 128.0f;
        }
    } else {
        std::cerr << "Unsupported bit depth: " << header.bitsPerSample << std::endl;
    }
}

double WavFile::getDurationSeconds() const {
    if (header.sampleRate == 0) return 0.0;
    return static_cast<double>(numSamples) / header.sampleRate;
}
