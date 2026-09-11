#pragma once

#include "WavFile.h"
#include <memory>
#include <atomic>
#include <mutex>
#include <cstdint>

class SamplePlayer {
public:
    SamplePlayer();
    ~SamplePlayer();

    /**
     * Load a WAV file for playback.
     * Returns true on success, false on error.
     */
    bool loadSample(const std::string& filePath);

    /**
     * Start playback from current position.
     */
    void play();

    /**
     * Stop playback.
     */
    void stop();

    /**
     * Seek to a specific time in seconds.
     */
    void seekTo(double timeSeconds);

    /**
     * Get current playback position in seconds.
     */
    double getCurrentPosition() const;

    /**
     * Get total duration in seconds.
     */
    double getDuration() const;

    /**
     * Get playback state.
     */
    bool isPlaying() const { return playing.load(); }

    /**
     * Check if a sample is loaded.
     */
    bool hasSample() const { return sample != nullptr && sample->isLoaded(); }

    /**
     * Get the next audio frame. Called from audio callback.
     * Fills buffer with interleaved float samples.
     * Returns number of frames written.
     */
    int32_t getAudioFrame(float* buffer, int32_t numFrames, int32_t numChannels);

private:
    std::unique_ptr<WavFile> sample;
    std::atomic<uint64_t> currentSampleIndex{0};
    std::atomic<bool> playing{false};
    std::mutex sampleMutex;

    /**
     * Resample or convert audio if needed.
     */
    void prepareAudioBuffer();
};
