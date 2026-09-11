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
    stopRecording();
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
    } else {
        std::cerr << "Failed to start stream: " << static_cast<int>(result) << std::endl;
    }
}

void AudioEngine::stop() {
    if (!isPlaying.load() || !stream) return;
    stream->requestStop();
    isPlaying.store(false);
}

bool AudioEngine::startRecording(const std::string &path) {
    if (!stream) {
        std::cerr << "Cannot start recording: stream not open" << std::endl;
        return false;
    }
    if (writerOpened) return false;

    int sr = static_cast<int>(stream->getSampleRate());
    int ch = stream->getChannelCount();
    bool ok = wavWriter.open(path, sr, ch);
    if (ok) {
        writerOpened = true;
        isRecording.store(true);
    }
    return ok;
}

void AudioEngine::stopRecording() {
    if (writerOpened) {
        wavWriter.close();
        writerOpened = false;
    }
    isRecording.store(false);
}

void AudioEngine::setRecording(bool enable) {
    isRecording.store(enable);
    if (!enable) {
        if (writerOpened) {
            wavWriter.close();
            writerOpened = false;
        }
    }
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

    // If recording, capture buffer to file
    if (isRecording.load() && writerOpened) {
        wavWriter.writeFloats(out, numFrames);
    }

    return DataCallbackResult::Continue;
}
