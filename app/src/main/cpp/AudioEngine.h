#pragma once
#include <oboe/Oboe.h>
#include <atomic>
#include "WAVWriter.h"
#include <string>
#include <vector>
#include <mutex>
#include <memory>
#include "Sample.h"

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

    // Sample management
    int loadSample(const std::string &path); // returns sample id or -1
    bool triggerSample(int sampleId, float gain = 1.0f);

    // Track management
    int createTrackWithSample(const std::string &path); // returns track id or -1
    bool setTrackGain(int trackId, float gain);
    bool toggleTrackMute(int trackId);

    // Thumbnail generation
    std::vector<float> getSampleThumbnail(int sampleId, int width);

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

    // samples and voices
    std::vector<std::shared_ptr<Sample>> samples;
    std::vector<struct Voice> voices;
    std::mutex samplesMutex;
    std::mutex voicesMutex;

    // tracks
    std::vector<Track> tracks;
    std::mutex tracksMutex;
};
