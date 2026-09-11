#pragma once
#include <string>
#include <vector>

struct Sample {
    int sampleRate = 0;
    int channels = 0;
    std::vector<float> data; // interleaved
};

// Loads 16-bit PCM WAV files only (simple parser)
bool loadWavFile(const std::string &path, Sample &outSample);
