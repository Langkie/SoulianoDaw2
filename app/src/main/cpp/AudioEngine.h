#pragma once
#include <oboe/Oboe.h>
#include <atomic>
#include <fstream>
#include <mutex>
#include <string>
#include <cstdint>
#include <memory>
#include "SamplePlayer.h"

class AudioEngine : public oboe::AudioStreamCallback {
public:
    AudioEngine();
    ~AudioEngine();

    void start();
    void stop();

    // Sample playback
    bool loadSample(const std::string& filePath);
    void playSample();
    void stopSample();
    void seekSample(double timeSeconds);
    double getSamplePosition();
    double getSampleDuration();
    bool hasSampleLoaded();

    // Recording control
    bool startRecording(const std::string &path);
    void stopRecording();

    // oboe::AudioStreamCallback
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream,
                                          void *audioData, int32_t numFrames) override;

private:
    oboe::AudioStream *stream = nullptr;
    std::atomic<bool> isPlaying{false};
    std::atomic<bool> isRecording{false};

    double phase = 0.0;
    double phaseIncrement = 2.0 * M_PI * 440.0 / 48000.0; // default 48k sample rate handling later

    // Sample player
    std::unique_ptr<SamplePlayer> samplePlayer;

    // WAV file output
    std::ofstream wavFile;
    std::mutex fileMutex;
    uint64_t totalFramesWritten = 0; // frames * channels
    int wavNumChannels = 2;
    int wavSampleRate = 48000;
};
