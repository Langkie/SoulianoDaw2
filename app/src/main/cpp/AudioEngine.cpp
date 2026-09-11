#include "AudioEngine.h"
#include "Sample.h"
#include <oboe/Oboe.h>
#include <thread>
#include <iostream>
#include <cmath>
#include <mutex>
#include <memory>
#include <vector>

using namespace oboe;

struct Voice {
    std::shared_ptr<Sample> sample;
    uint64_t position = 0; // in frames
    float gain = 1.0f;
    bool finished = false;
};

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

int AudioEngine::loadSample(const std::string &path) {
    std::shared_ptr<Sample> s = std::make_shared<Sample>();
    if (!loadWavFile(path, *s)) return -1;
    std::lock_guard<std::mutex> lock(samplesMutex);
    samples.push_back(s);
    return static_cast<int>(samples.size() - 1);
}

bool AudioEngine::triggerSample(int sampleId, float gain) {
    std::lock_guard<std::mutex> lock(samplesMutex);
    if (sampleId < 0 || sampleId >= static_cast<int>(samples.size())) return false;
    Voice v;
    v.sample = samples[sampleId];
    v.position = 0;
    v.gain = gain;
    std::lock_guard<std::mutex> lock2(voicesMutex);
    voices.push_back(v);
    return true;
}

int AudioEngine::createTrackWithSample(const std::string &path) {
    int sampleId = loadSample(path);
    if (sampleId < 0) return -1;
    Track t;
    t.sampleId = sampleId;
    t.gain = 1.0f;
    t.muted = false;
    t.solo = false;
    std::lock_guard<std::mutex> lock(tracksMutex);
    tracks.push_back(t);
    return static_cast<int>(tracks.size() - 1);
}

bool AudioEngine::setTrackGain(int trackId, float gain) {
    std::lock_guard<std::mutex> lock(tracksMutex);
    if (trackId < 0 || trackId >= static_cast<int>(tracks.size())) return false;
    tracks[trackId].gain = gain;
    return true;
}

bool AudioEngine::toggleTrackMute(int trackId) {
    std::lock_guard<std::mutex> lock(tracksMutex);
    if (trackId < 0 || trackId >= static_cast<int>(tracks.size())) return false;
    tracks[trackId].muted = !tracks[trackId].muted;
    return true;
}

DataCallbackResult AudioEngine::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
    float *out = static_cast<float*>(audioData);
    int32_t numChannels = oboeStream->getChannelCount();

    // zero output
    for (int i = 0; i < numFrames * numChannels; ++i) out[i] = 0.0f;

    // simple oscillator (for testing)
    float amplitude = 0.12f;
    double sr = oboeStream->getSampleRate();
    if (sr > 0) phaseIncrement = 2.0 * M_PI * 440.0 / sr;

    for (int i = 0; i < numFrames; ++i) {
        float oscValue = static_cast<float>(sin(phase) * amplitude);
        phase += phaseIncrement;
        if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;
        for (int c = 0; c < numChannels; ++c) {
            out[i * numChannels + c] += oscValue;
        }
    }

    // Mix active voices
    std::lock_guard<std::mutex> lock(voicesMutex);
    for (auto it = voices.begin(); it != voices.end();) {
        Voice &v = *it;
        auto s = v.sample;
        if (!s) { it = voices.erase(it); continue; }
        uint64_t framesInSample = s->data.size() / s->channels;

        for (int i = 0; i < numFrames; ++i) {
            if (v.position >= framesInSample) break;
            for (int c = 0; c < numChannels; ++c) {
                int sampleChannel = c < s->channels ? c : 0;
                float sampleValue = s->data[v.position * s->channels + sampleChannel];
                out[i * numChannels + c] += sampleValue * v.gain;
            }
            v.position++;
        }

        if (v.position >= framesInSample) {
            it = voices.erase(it);
        } else {
            ++it;
        }
    }

    // Mix tracks' head-of-sample once (tracks are represented as metadata; triggering is separate)
    // Currently, tracks only hold metadata (gain/mute) — playback is done via triggerSample which creates voices.

    // If recording, capture buffer to file
    if (isRecording.load() && writerOpened) {
        wavWriter.writeFloats(out, numFrames);
    }

    return DataCallbackResult::Continue;
}
