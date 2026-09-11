#include "SamplePlayer.h"
#include <iostream>
#include <algorithm>
#include <cstring>

SamplePlayer::SamplePlayer() = default;

SamplePlayer::~SamplePlayer() {
    stop();
}

bool SamplePlayer::loadSample(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(sampleMutex);
    
    auto newSample = std::make_unique<WavFile>();
    if (!newSample->load(filePath)) {
        std::cerr << "Failed to load sample: " << filePath << std::endl;
        return false;
    }

    sample = std::move(newSample);
    currentSampleIndex.store(0);
    playing.store(false);
    
    std::cerr << "Sample loaded successfully. Duration: " << getDuration() << "s" << std::endl;
    return true;
}

void SamplePlayer::play() {
    playing.store(true);
    std::cerr << "Sample playback started" << std::endl;
}

void SamplePlayer::stop() {
    playing.store(false);
    std::cerr << "Sample playback stopped" << std::endl;
}

void SamplePlayer::seekTo(double timeSeconds) {
    if (!sample) return;

    uint64_t sampleIndex = static_cast<uint64_t>(timeSeconds * sample->getHeader().sampleRate);
    sampleIndex = std::min(sampleIndex, static_cast<uint64_t>(sample->getNumSamples()));
    currentSampleIndex.store(sampleIndex);

    std::cerr << "Seeked to " << timeSeconds << "s (sample index: " << sampleIndex << ")" << std::endl;
}

double SamplePlayer::getCurrentPosition() const {
    if (!sample) return 0.0;
    
    uint64_t index = currentSampleIndex.load();
    return static_cast<double>(index) / sample->getHeader().sampleRate;
}

double SamplePlayer::getDuration() const {
    if (!sample) return 0.0;
    return sample->getDurationSeconds();
}

int32_t SamplePlayer::getAudioFrame(float* buffer, int32_t numFrames, int32_t numChannels) {
    if (!sample || !playing.load()) {
        // Fill with silence
        std::memset(buffer, 0, numFrames * numChannels * sizeof(float));
        return numFrames;
    }

    std::lock_guard<std::mutex> lock(sampleMutex);

    if (!sample->isLoaded()) {
        std::memset(buffer, 0, numFrames * numChannels * sizeof(float));
        return numFrames;
    }

    const float* audioData = sample->getAudioData();
    uint32_t totalSamples = sample->getNumSamples() * sample->getHeader().numChannels;
    int32_t sampleChannels = sample->getHeader().numChannels;

    uint64_t currentIndex = currentSampleIndex.load();
    int32_t framesWritten = 0;

    for (int32_t i = 0; i < numFrames; ++i) {
        if (currentIndex >= sample->getNumSamples()) {
            // End of sample reached
            playing.store(false);
            for (int32_t c = 0; c < numChannels; ++c) {
                buffer[i * numChannels + c] = 0.0f;
            }
            framesWritten++;
            continue;
        }

        // Read from sample and mix to output channels
        for (int32_t c = 0; c < numChannels; ++c) {
            if (c < sampleChannels) {
                // Use sample's channel
                uint64_t samplePos = currentIndex * sampleChannels + c;
                buffer[i * numChannels + c] = audioData[samplePos];
            } else {
                // Extra output channels get silence
                buffer[i * numChannels + c] = 0.0f;
            }
        }

        currentIndex++;
        framesWritten++;
    }

    currentSampleIndex.store(currentIndex);
    return framesWritten;
}
