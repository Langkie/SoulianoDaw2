#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

struct WavHeader {
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    uint32_t dataSize;
};

class WavFile {
public:
    WavFile();
    ~WavFile();

    /**
     * Load a WAV file from the given path.
     * Returns true on success, false on error.
     */
    bool load(const std::string& filePath);

    /**
     * Get the WAV header information.
     */
    const WavHeader& getHeader() const { return header; }

    /**
     * Get pointer to raw audio data (float samples).
     */
    const float* getAudioData() const { return audioData.data(); }

    /**
     * Get total number of samples (per channel).
     */
    uint32_t getNumSamples() const { return numSamples; }

    /**
     * Get duration in seconds.
     */
    double getDurationSeconds() const;

    /**
     * Check if file is loaded.
     */
    bool isLoaded() const { return !audioData.empty(); }

private:
    WavHeader header{};
    std::vector<float> audioData;
    uint32_t numSamples = 0;

    /**
     * Helper to read and validate WAV file format.
     */
    bool readWavHeader(FILE* file);

    /**
     * Convert raw PCM data to float samples.
     */
    void convertToFloat(const std::vector<uint8_t>& rawData);
};
