#include "AudioEngine.h"
#include <oboe/Oboe.h>
#include <thread>
#include <iostream>
#include <fstream>
#include <cstring>

using namespace oboe;

static constexpr size_t kMaxQueueBuffers = 512;

static void writeLE(std::ofstream &out, uint32_t value) {
    uint8_t bytes[4];
    bytes[0] = value & 0xff;
    bytes[1] = (value >> 8) & 0xff;
    bytes[2] = (value >> 16) & 0xff;
    bytes[3] = (value >> 24) & 0xff;
    out.write(reinterpret_cast<char*>(bytes), 4);
}

static void writeLE16(std::ofstream &out, uint16_t value) {
    uint8_t bytes[2];
    bytes[0] = value & 0xff;
    bytes[1] = (value >> 8) & 0xff;
    out.write(reinterpret_cast<char*>(bytes), 2);
}

AudioEngine::AudioEngine() {
    // empty
}

AudioEngine::~AudioEngine() {
    stop();
    // ensure writer stopped
    stopRecording();
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
    std::lock_guard<std::mutex> lock(fileMutex);

    if (isRecording.load()) return false;

    wavFile.open(path, std::ios::binary | std::ios::out);
    if (!wavFile.is_open()) {
        std::cerr << "Failed to open WAV file for writing: " << path << std::endl;
        return false;
    }

    // write placeholder WAV header (44 bytes)
    wavFile.write("RIFF", 4);
    writeLE(wavFile, 0); // placeholder for chunk size
    wavFile.write("WAVE", 4);
    wavFile.write("fmt ", 4);
    writeLE(wavFile, 16); // PCM subchunk size
    writeLE16(wavFile, 1); // audio format = 1 (PCM)
    writeLE16(wavFile, static_cast<uint16_t>(wavNumChannels));
    writeLE(wavFile, static_cast<uint32_t>(wavSampleRate));
    uint32_t byteRate = wavSampleRate * wavNumChannels * sizeof(int16_t);
    writeLE(wavFile, byteRate);
    writeLE16(wavFile, static_cast<uint16_t>(wavNumChannels * sizeof(int16_t))); // block align
    writeLE16(wavFile, 16); // bits per sample
    wavFile.write("data", 4);
    writeLE(wavFile, 0); // placeholder for data size
    wavFile.flush();

    totalFramesWritten = 0;
    writerActive.store(true);
    isRecording.store(true);

    // start writer thread
    writerThread = std::thread([this]() {
        while (writerActive.load() || !bufferQueue.empty()) {
            std::vector<float> buf;
            {
                std::unique_lock<std::mutex> qlock(queueMutex);
                if (bufferQueue.empty()) {
                    queueCond.wait_for(qlock, std::chrono::milliseconds(100));
                }
                if (!bufferQueue.empty()) {
                    buf = std::move(bufferQueue.front());
                    bufferQueue.pop_front();
                }
            }

            if (!buf.empty()) {
                // convert float buffer to int16 and write
                std::vector<int16_t> pcm;
                pcm.reserve(buf.size());
                for (size_t i = 0; i < buf.size(); ++i) {
                    float f = buf[i];
                    // simple clipping
                    if (f > 1.0f) f = 1.0f;
                    if (f < -1.0f) f = -1.0f;
                    int16_t s = static_cast<int16_t>(f * 32767.0f);
                    pcm.push_back(s);
                }

                // write to file
                {
                    std::lock_guard<std::mutex> lock(fileMutex);
                    wavFile.write(reinterpret_cast<char*>(pcm.data()), pcm.size() * sizeof(int16_t));
                    totalFramesWritten += (pcm.size() / wavNumChannels);
                }
            }
        }

        // final flush
        {
            std::lock_guard<std::mutex> lock(fileMutex);
            wavFile.flush();
        }
    });

    return true;
}

void AudioEngine::stopRecording() {
    // stop accepting new buffers
    isRecording.store(false);
    writerActive.store(false);
    queueCond.notify_all();

    if (writerThread.joinable()) {
        writerThread.join();
    }

    std::lock_guard<std::mutex> lock(fileMutex);
    if (!wavFile.is_open()) return;

    // finalize header: update chunk sizes
    uint32_t dataBytes = static_cast<uint32_t>(totalFramesWritten * wavNumChannels * sizeof(int16_t));
    uint32_t riffChunkSize = 36 + dataBytes;

    wavFile.seekp(4, std::ios::beg);
    writeLE(wavFile, riffChunkSize);

    wavFile.seekp(40, std::ios::beg);
    writeLE(wavFile, dataBytes);

    wavFile.close();
}

DataCallbackResult AudioEngine::onAudioReady(AudioStream *oboeStream, void *audioData, int32_t numFrames) {
    float *out = static_cast<float*>(audioData);
    int32_t numChannels = oboeStream->getChannelCount();
    float amplitude = 0.2f;

    double sr = oboeStream->getSampleRate();
    if (sr > 0) phaseIncrement = 2.0 * M_PI * 440.0 / sr;
    wavSampleRate = static_cast<int>(sr > 0 ? sr : wavSampleRate);

    // generate audio into out buffer and optionally capture
    size_t totalSamples = static_cast<size_t>(numFrames) * static_cast<size_t>(numChannels);
    std::vector<float> captureBuffer;
    captureBuffer.reserve(totalSamples);

    for (int i = 0; i < numFrames; ++i) {
        float value = static_cast<float>(sin(phase) * amplitude);
        phase += phaseIncrement;
        if (phase >= 2.0 * M_PI) phase -= 2.0 * M_PI;

        for (int c = 0; c < numChannels; ++c) {
            float sample = value;
            out[i * numChannels + c] = sample;
            if (isRecording.load()) captureBuffer.push_back(sample);
        }
    }

    // enqueue captured buffer if recording
    if (isRecording.load() && !captureBuffer.empty()) {
        std::lock_guard<std::mutex> qlock(queueMutex);
        if (bufferQueue.size() < maxQueuedBuffers) {
            bufferQueue.emplace_back(std::move(captureBuffer));
            queueCond.notify_one();
        } else {
            // drop if queue full
        }
    }

    return DataCallbackResult::Continue;
}
