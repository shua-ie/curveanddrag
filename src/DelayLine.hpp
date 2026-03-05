#pragma once
#include <rack.hpp>
#include <vector>
#include <cmath>

namespace CurveAndDrag {

/**
 * DelayLine - Class for implementing a delay line with interpolation
 *
 * Features smooth delay time changes, feedback, and cross-feedback options.
 * Supports both internal feedback (classic) and external feedback processing
 * (for pitch-in-feedback-loop / shimmer architecture).
 */
class DelayLine {
public:
    DelayLine() {
        sampleRate = 44100.0f;
        maxDelayTimeMs = 2000.0f;
        delayTimeMs = 100.0f;
        feedback = 0.5f;
        dryWet = 0.5f;
        reset();
    }

    void reset() {
        int bufferSize = static_cast<int>(std::ceil((maxDelayTimeMs / 1000.0f) * sampleRate)) + 4;
        buffer.resize(bufferSize, 0.0f);
        writeIndex = 0;
        delayInSamples = (delayTimeMs / 1000.0f) * sampleRate;
    }

    void configure(float newSampleRate) {
        if (sampleRate != newSampleRate) {
            sampleRate = newSampleRate;
            reset();
        }
    }

    void setDelayTime(float newDelayTimeMs) {
        delayTimeMs = rack::math::clamp(newDelayTimeMs, 1.0f, maxDelayTimeMs);
        delayInSamples = (delayTimeMs / 1000.0f) * sampleRate;
    }

    void setFeedback(float newFeedback) {
        feedback = rack::math::clamp(newFeedback, 0.0f, 1.1f);
    }

    void setDryWet(float newDryWet) {
        dryWet = rack::math::clamp(newDryWet, 0.0f, 1.0f);
    }

    float getFeedback() const { return feedback; }
    float getDryWet() const { return dryWet; }

    /**
     * Classic process: read, apply internal feedback, write, mix dry/wet.
     * Use this when pitch is OUTSIDE the feedback loop (pre-loop mode).
     */
    float process(float input, float externalFeedback = 0.0f) {
        float delayedSample = read();
        float feedbackSignal = feedback * delayedSample + externalFeedback;
        write(input + feedbackSignal);
        return input * (1.0f - dryWet) + delayedSample * dryWet;
    }

    // === Separated feedback path for pitch-in-loop architecture ===

    /**
     * Read the current delayed sample (does NOT advance write pointer).
     * Use with writeWithFeedback() for external feedback processing.
     */
    float readDelayed() {
        return read();
    }

    /**
     * Write input + externally-processed feedback signal.
     * Call after readDelayed() + external processing (pitch shift, filter, etc.)
     *
     * @param input Fresh input sample
     * @param processedFeedback The delayed signal after external processing * feedback amount
     */
    void writeWithFeedback(float input, float processedFeedback) {
        write(input + processedFeedback);
    }

    /**
     * Get the delayed signal only (no dry/wet mixing)
     */
    float getDelayedSignal() {
        return read();
    }

    /**
     * Get the wet signal only
     */
    float getWetSignal() {
        return read();
    }

    /**
     * Get current buffer contents for freeze mode - captures the buffer state
     */
    void captureBuffer(std::vector<float>& dest) const {
        dest = buffer;
    }

    /**
     * Restore buffer from captured state (for freeze release)
     */
    void restoreBuffer(const std::vector<float>& src) {
        if (src.size() == buffer.size()) {
            buffer = src;
        }
    }

    /**
     * Get buffer size for freeze operations
     */
    int getBufferSize() const { return static_cast<int>(buffer.size()); }

    /**
     * Get current write index
     */
    int getWriteIndex() const { return writeIndex; }

private:
    float sampleRate;
    float maxDelayTimeMs;
    float delayTimeMs;
    float delayInSamples;
    float feedback;
    float dryWet;

    std::vector<float> buffer;
    int writeIndex;

    float read() {
        float readPos = writeIndex - delayInSamples;
        if (readPos < 0) {
            readPos += buffer.size();
        }

        int readPos_i = static_cast<int>(readPos);
        float frac = readPos - readPos_i;
        int bufSize = static_cast<int>(buffer.size());

        float p0 = buffer[(readPos_i - 1 + bufSize) % bufSize];
        float p1 = buffer[readPos_i];
        float p2 = buffer[(readPos_i + 1) % bufSize];
        float p3 = buffer[(readPos_i + 2) % bufSize];

        float c0 = p1;
        float c1 = 0.5f * (p2 - p0);
        float c2 = p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3;
        float c3 = 0.5f * (p3 - p0) + 1.5f * (p1 - p2);
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }

    void write(float sample) {
        buffer[writeIndex] = sample;
        writeIndex = (writeIndex + 1) % buffer.size();
    }
};

} // namespace CurveAndDrag
