bool AudioEngine::exportMixdown(const std::string &path, float targetSampleRate) {
    // Determine mix length in frames (based on samples referenced by tracks)
    size_t maxFrames = 0;
    int outChannels = 2; // stereo output

    {
        std::lock_guard<std::mutex> lock(tracksMutex);
        std::lock_guard<std::mutex> lockS(samplesMutex);
        for (const auto &t : tracks) {
            if (t.sampleId < 0 || t.sampleId >= static_cast<int>(samples.size())) continue;
            auto s = samples[t.sampleId];
            if (!s) continue;
            size_t frames = s->data.size() / s->channels;
            if (frames > maxFrames) maxFrames = frames;
        }
    }

    if (maxFrames == 0) return false;

    // Prepare interleaved mix buffer in chunks to avoid huge memory usage
    // We'll write in blocks of 2048 frames
    const size_t blockFrames = 2048;

    WAVWriter writer;
    if (!writer.open(path, static_cast<int>(targetSampleRate), outChannels)) {
        std::cerr << "Failed to open mixdown file: " << path << std::endl;
        return false;
    }

    // For simplicity, we assume all samples have same sample rate as stream or targetSampleRate.
    // A proper implementation should resample differing sample rates.

    size_t writtenFrames = 0;
    std::vector<float> mixBuffer(blockFrames * outChannels);

    while (writtenFrames < maxFrames) {
        size_t framesThis = std::min(blockFrames, maxFrames - writtenFrames);
        // zero mix buffer
        std::fill(mixBuffer.begin(), mixBuffer.end(), 0.0f);

        // mix each track
        std::lock_guard<std::mutex> lockT(tracksMutex);
        std::lock_guard<std::mutex> lockS(samplesMutex);
        for (const auto &t : tracks) {
            if (t.muted) continue;
            if (t.sampleId < 0 || t.sampleId >= static_cast<int>(samples.size())) continue;
            auto s = samples[t.sampleId];
            if (!s) continue;
            size_t sampleFrames = s->data.size() / s->channels;
            for (size_t f = 0; f < framesThis; ++f) {
                size_t frameIndex = writtenFrames + f;
                if (frameIndex >= sampleFrames) break;
                for (int c = 0; c < outChannels; ++c) {
                    int sampleChannel = c < s->channels ? c : 0;
                    float sampleValue = s->data[frameIndex * s->channels + sampleChannel];
                    mixBuffer[f * outChannels + c] += sampleValue * t.gain;
                }
            }
        }

        // write mixBuffer to WAV (writer expects interleaved floats)
        writer.writeFloats(mixBuffer.data(), static_cast<int>(framesThis));
        writtenFrames += framesThis;
    }

    writer.close();
    std::cout << "Mixdown written to: " << path << std::endl;
    return true;
}
