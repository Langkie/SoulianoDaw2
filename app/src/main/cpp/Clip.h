#pragma once
#include <cstdint>

struct Clip {
    int sampleId = -1; // which sample to play
    int trackId = -1;
    uint64_t startFrame = 0; // transport frame where clip starts
    uint64_t lengthFrames = 0; // number of frames in clip
};
