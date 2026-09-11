#include "AudioEngine.h"
#include "Sample.h"
#include <oboe/Oboe.h>
#include <thread>
#include <iostream>
#include <cmath>
#include <mutex>
#include <memory>
#include <vector>
#include <algorithm>
#include <chrono>

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
    stopRecordingToTrack();
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
        // start transport at 0 by default
        transportFrame.store(0);
        transportPlaying.store(true);
    } else {
        std::cerr << "Failed to start stream: " << static_cast<int>(result) << std::endl;
    }
}

void AudioEngine::stop() {
    if (!isPlaying.load() || !stream) return;
    stream->requestStop();
    isPlaying.store(false);
    transportPlaying.store(false);
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

bool AudioEngine::startRecordingToTrack(int trackId) {
    if (!stream) {
        std::cerr << "Cannot start track recording: stream not open" << std::endl;
        return false;
    }
    std::lock_guard<std::mutex> lock(recordingMutex);
    if (recordingToTrack.load()) return false;
    recordingBuffer.clear();
    recordingTrackId = trackId;
    recordingStartTransportFrame = transportFrame.load();
    recordingToTrack.store(true);
    std::cout << "Started recording to track " << trackId << " at transport frame " << recordingStartTransportFrame << std::endl;
    return true;
}

bool AudioEngine::stopRecordingToTrack() {
    std::lock_guard<std::mutex> lock(recordingMutex);
    if (!recordingToTrack.load()) return false;
    recordingToTrack.store(false);

    // convert recordingBuffer to a Sample and add to samples
    std::shared_ptr<Sample> s = std::make_shared<Sample>();
    if (!stream) return false;
    s->sampleRate = static_cast<int>(stream->getSampleRate());
    // assume stereo
    int channels = stream->getChannelCount();
    s->channels = channels;
    s->data = recordingBuffer; // already interleaved floats

    int sampleId = -1;
    {
        std::lock_guard<std::mutex> lockSamples(samplesMutex);
        samples.push_back(s);
        sampleId = static_cast<int>(samples.size() - 1);
    }

    // create a track if necessary
    int trackId = recordingTrackId;
    {
        std::lock_guard<std::mutex> lockTracks(tracksMutex);
        if (trackId < 0 || trackId >= static_cast<int>(tracks.size())) {
            Track t;
            t.sampleId = sampleId;
            t.gain = 1.0f;
            t.muted = false;
            t.solo = false;
            tracks.push_back(t);
            trackId = static_cast<int>(tracks.size() - 1);
        } else {
            // set track's sample to this new sample
            tracks[trackId].sampleId = sampleId;
        }
    }

    // create a clip referencing this sample starting at recordingStartTransportFrame
    uint64_t startFrame = recordingStartTransportFrame;
    int clipId = -1;
    {
        Clip c;
        c.sampleId = sampleId;
        c.trackId = trackId;
        c.startFrame = startFrame;
        c.lengthFrames = s->data.size() / s->channels;
        std::lock_guard<std::mutex> lockClips(clipsMutex);
        clips.push_back(c);
        clipId = static_cast<int>(clips.size() - 1);
    }

    // Optionally write out WAV file copy to external recordings dir
    // we will skip file writing here to keep things fast; callers can export mixdown later

    std::cout << "Stopped recording to track " << trackId << ", created sample " << sampleId << " and clip " << clipId << std::endl;
    recordingTrackId = -1;
    recordingBuffer.clear();
    return true;
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

int AudioEngine::createClipFromSample(int sampleId, int trackId, uint64_t startFrame) {
    std::shared_ptr<Sample> s;
    {
        std::lock_guard<std::mutex> lock(samplesMutex);
        if (sampleId < 0 || sampleId >= static_cast<int>(samples.size())) return -1;
        s = samples[sampleId];
    }
    if (!s) return -1;
    Clip c;
    c.sampleId = sampleId;
    c.trackId = trackId;
    c.startFrame = startFrame;
    c.lengthFrames = s->data.size() / s->channels;
    std::lock_guard<std::mutex> lock(clipsMutex);
    clips.push_back(c);
    return static_cast<int>(clips.size() - 1);
}

void AudioEngine::setTransportPlay(bool play) {
    transportPlaying.store(play);
}

void AudioEngine::seekTransport(uint64_t frame) {
    transportFrame.store(frame);
}

std::vector<float> AudioEngine::getSampleThumbnail(int sampleId, int width) {
    std::vector<float> out;
    if (width <= 0) return out;
    std::shared_ptr<Sample> s;
    {
        std::lock_guard<std::mutex> lock(samplesMutex);
        if (sampleId < 0 || sampleId >= static_cast<int>(samples.size())) return out;
        s = samples[sampleId];
    }
    if (!s) return out;
    size_t frames = s->data.size() / s->channels;
    if (frames == 0) return out;

    out.resize(width);
    size_t samplesPerBucket = std::max<size_t>(1, frames / width);
    for (int i = 0; i < width; ++i) {
        size_t startFrame = i * samplesPerBucket;
        size_t endFrame = std::min(frames, startFrame + samplesPerBucket);
        float peak = 0.0f;
        for (size_t f = startFrame; f < endFrame; ++f) {
            for (int c = 0; c < s->channels; ++c) {
                float v = s->data[f * s->channels + c];
                peak = std::max(peak, fabsf(v));
            }
        }
        out[i] = peak; // normalized [0..1]
    }
    return out;
}

DataCallbackResult AudioEngine::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
    float *out = static_cast<float*>(audioData);
    int32_t numChannels = oboeStream->getChannelCount();

    // zero output
    for (int i = 0; i < numFrames * numChannels; ++i) out[i] = 0.0f;

    // simple oscillator (for testing)
    float amplitude = 0.08f;
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

    // Mix clips based on transport
    uint64_t currentTransport = transportFrame.load();
    bool playingTransport = transportPlaying.load();

    if (playingTransport) {
        std::lock_guard<std::mutex> lockClips(clipsMutex);
        std::lock_guard<std::mutex> lockSamples(samplesMutex);
        for (const auto &clip : clips) {
            // If transport overlaps clip range, mix overlapping frames
            uint64_t clipStart = clip.startFrame;
            uint64_t clipEnd = clip.startFrame + clip.lengthFrames;
            uint64_t blockStart = currentTransport;
            uint64_t blockEnd = currentTransport + static_cast<uint64_t>(numFrames);

            if (blockEnd <= clipStart || blockStart >= clipEnd) continue; // no overlap

            // compute overlap range
            uint64_t mixStart = std::max(blockStart, clipStart);
            uint64_t mixEnd = std::min(blockEnd, clipEnd);

            int sampleId = clip.sampleId;
            if (sampleId < 0 || sampleId >= static_cast<int>(samples.size())) continue;
            auto s = samples[sampleId];
            if (!s) continue;
            for (uint64_t f = mixStart; f < mixEnd; ++f) {
                uint64_t frameIndexInBlock = f - blockStart; // 0..numFrames-1
                uint64_t frameIndexInClip = f - clipStart; // index into sample
                if (frameIndexInClip >= clip.lengthFrames) break;
                for (int c = 0; c < numChannels; ++c) {
                    int sampleChannel = c < s->channels ? c : 0;
                    float sampleValue = s->data[frameIndexInClip * s->channels + sampleChannel];
                    // find track gain
                    float trackGain = 1.0f;
                    bool muted = false;
                    if (clip.trackId >= 0 && clip.trackId < static_cast<int>(tracks.size())) {
                        trackGain = tracks[clip.trackId].gain;
                        muted = tracks[clip.trackId].muted;
                    }
                    if (!muted)
                        out[static_cast<int>(frameIndexInBlock) * numChannels + c] += sampleValue * trackGain;
                }
            }
        }

        transportFrame.fetch_add(static_cast<uint64_t>(numFrames));
    }

    // Mix active voices (oneshot triggers)
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

    // If recording, capture buffer to file
    if (isRecording.load() && writerOpened) {
        wavWriter.writeFloats(out, numFrames);
    }

    // If recording to track, copy output block into recording buffer
    if (recordingToTrack.load()) {
        std::lock_guard<std::mutex> lock(recordingMutex);
        // append interleaved floats
        recordingBuffer.insert(recordingBuffer.end(), out, out + (numFrames * numChannels));
    }

    return DataCallbackResult::Continue;
}
