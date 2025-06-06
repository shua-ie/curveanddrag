#pragma once
#include <rack.hpp>
#include <vector>
#include <cmath>

namespace CurveAndDrag {

/**
 * DelayLine - Class for implementing a delay line with interpolation
 * 
 * Features smooth delay time changes, feedback, and cross-feedback options
 */
class DelayLine {
public:
    DelayLine() {
        // Initialize with default values
        sampleRate = 44100.0f;
        maxDelayTimeMs = 2000.0f;  // 2 seconds maximum delay
        delayTimeMs = 100.0f;      // Default 100ms
        feedback = 0.5f;           // Default 50% feedback
        dryWet = 0.5f;             // Default 50/50 dry/wet
        
        // Initialize buffers
        reset();
    }

    /**
     * Reset all internal buffer state
     */
    void reset() {
        // Calculate buffer size based on max delay time
        int bufferSize = static_cast<int>(std::ceil((maxDelayTimeMs / 1000.0f) * sampleRate)) + 2;
        buffer.resize(bufferSize, 0.0f);
        writeIndex = 0;
        
        // Calculate read position based on current delay time
        delayInSamples = (delayTimeMs / 1000.0f) * sampleRate;
    }

    /**
     * Configure the delay line
     * 
     * @param newSampleRate Audio sample rate
     */
    void configure(float newSampleRate) {
        if (sampleRate != newSampleRate) {
            sampleRate = newSampleRate;
            reset();
        }
    }

    /**
     * Set the delay time in milliseconds
     * 
     * @param newDelayTimeMs Delay time in milliseconds (1-2000 ms)
     */
    void setDelayTime(float newDelayTimeMs) {
        // Constrain to valid range
        delayTimeMs = rack::math::clamp(newDelayTimeMs, 1.0f, maxDelayTimeMs);
        delayInSamples = (delayTimeMs / 1000.0f) * sampleRate;
    }

    /**
     * Set the feedback amount
     * 
     * @param newFeedback Feedback amount (0.0-1.1)
     */
    void setFeedback(float newFeedback) {
        // Allow feedback up to 110%, but don't exceed that to prevent runaway
        feedback = rack::math::clamp(newFeedback, 0.0f, 1.1f);
    }

    /**
     * Set the dry/wet mix
     * 
     * @param newDryWet Dry/wet mix amount (0.0-1.0)
     */
    void setDryWet(float newDryWet) {
        dryWet = rack::math::clamp(newDryWet, 0.0f, 1.0f);
    }

    /**
     * Process a single audio sample through the delay line
     * 
     * @param input Input audio sample
     * @param externalFeedback Optional external feedback signal (for cross-feedback)
     * @return Processed output sample
     */
    float process(float input, float externalFeedback = 0.0f) {
        // Read from delay line with linear interpolation
        float delayedSample = read();
        
        // Apply feedback with optional external signal
        float feedbackSignal = feedback * delayedSample + externalFeedback;
        
        // Write to delay line
        write(input + feedbackSignal);
        
        // Mix dry and wet signals
        return input * (1.0f - dryWet) + delayedSample * dryWet;
    }

    /**
     * Get the delayed signal only (no dry/wet mixing)
     * 
     * @return The current delayed output sample
     */
    float getDelayedSignal() {
        return read();
    }

    /**
     * Get the wet signal only (delayed signal with feedback)
     * 
     * @return The current wet output sample
     */
    float getWetSignal() {
        return read(); // Return pure delayed signal without dry/wet mixing
    }

private:
    float sampleRate;
    float maxDelayTimeMs;
    float delayTimeMs;
    float delayInSamples;
    float feedback;
    float dryWet;
    
    std::vector<float> buffer;
    int writeIndex;

    /**
     * Read from the delay line with linear interpolation
     * 
     * @return Interpolated sample from delay line
     */
    float read() {
        // Calculate read position
        float readPos = writeIndex - delayInSamples;
        if (readPos < 0) {
            readPos += buffer.size();
        }
        
        // Get integer position and fraction for interpolation
        int readPos_i = static_cast<int>(readPos);
        float frac = readPos - readPos_i;
        
        // Get samples for interpolation
        int nextPos = (readPos_i + 1) % buffer.size();
        float sample1 = buffer[readPos_i];
        float sample2 = buffer[nextPos];
        
        // Linear interpolation
        return sample1 + frac * (sample2 - sample1);
    }

    /**
     * Write a sample to the delay line
     * 
     * @param sample Audio sample to write
     */
    void write(float sample) {
        buffer[writeIndex] = sample;
        writeIndex = (writeIndex + 1) % buffer.size();
    }
};

} // namespace CurveAndDrag
