#pragma once
#include <string>
#include <vector>

struct Sample {
    int sampleRate = 0;
    int channels = 0;
    std::vector<float> data; // interleaved
};

struct Track {
    int sampleId = -1; // index into AudioEngine's samples vector
    float gain = 1.0f;
    bool muted = false;
    bool solo = false;
    // playback position will be handled by voices when triggered
};

// Loads 16-bit PCM WAV files only (simple parser)
bool loadWavFile(const std::string &path, Sample &outSample);
