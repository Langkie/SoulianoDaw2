#pragma once
#include <oboe/Oboe.h>
#include <atomic>
#include "WAVWriter.h"

class AudioEngine : public oboe::AudioStreamCallback {
public:
    AudioEngine();
    ~AudioEngine();

    void start();
    void stop();

    // Recording control
    bool startRecording(const std::string &path);
    void stopRecording();
    void setRecording(bool enable);

    // oboe::AudioStreamCallback
    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream,
                                          void *audioData, int32_t numFrames) override;

private:
    oboe::AudioStream *stream = nullptr;
    std::atomic<bool> isPlaying{false};
    std::atomic<bool> isRecording{false};

    WAVWriter wavWriter;
    bool writerOpened = false;

    double phase = 0.0;
    double phaseIncrement = 2.0 * M_PI * 440.0 / 48000.0; // default 44.1/48k sample rate handling later
};
