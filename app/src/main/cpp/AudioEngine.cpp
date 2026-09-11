#include "AudioEngine.h"
#include <oboe/Oboe.h>
#include <thread>
#include <iostream>
#include <cmath>

using namespace oboe;

AudioEngine::AudioEngine() {
    // empty
}

AudioEngine::~AudioEngine() {
    stop();
    if (stream) {
        stream->close();
        stream = nullptr;
    }
}

void AudioEngine::start() {
    if (isPlaying.load()) return;

    AudioStreamBuilder builder;
    builder.setFormat(AudioFormat::Float)
           .setPerformanceMode(PerformanceMode::LowLatency)
           .setSharingMode(SharingMode::Exclusive)
           .setCallback(this)
           .setChannelCount(ChannelCount::Stereo)
           .setSampleRate(48000);

    Result result = builder.openStream(&stream);
    if (result != Result::OK) {
        std::cerr << "Failed to open Oboe stream: " << static_cast<int>(result) << std::endl;
        return;
    }

    result = stream->requestStart();
    if (result == Result::OK) {
        isPlaying.store(true);
        std::cerr << "Audio stream started successfully" << std::endl;
    } else {
        std::cerr << "Failed to start stream: " << static_cast<int>(result) << std::endl;
    }
}

void AudioEngine::stop() {
    if (!isPlaying.load() || !stream) return;
    stream->requestStop();
    isPlaying.store(false);
    std::cerr << "Audio stream stopped" << std::endl;
}

void AudioEngine::setRecording(bool enable) {
    isRecording.store(enable);
    if (enable) {
        std::cerr << "Recording enabled" << std::endl;
    } else {
        std::cerr << "Recording disabled" << std::endl;
    }
    // WAV capture + file writing to be implemented next
}

DataCallbackResult AudioEngine::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
    float *out = static_cast<float*>(audioData);
    int32_t numChannels = oboeStream->getChannelCount();
    float amplitude = 0.2f;

    double sr = oboeStream->getSampleRate();
    if (sr > 0) phaseIncrement = 2.0 * M_PI * 440.0 / sr;

    for (int i = 0; i < numFrames; ++i) {
        float value = static_cast<float>(sin(phase) * amplitude);
        phase += phaseIncrement;
        if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;

        for (int c = 0; c < numChannels; ++c) {
            out[i * numChannels + c] = value;
        }
    }

    // If recording, capture buffer to file (not implemented yet)

    return DataCallbackResult::Continue;
}
