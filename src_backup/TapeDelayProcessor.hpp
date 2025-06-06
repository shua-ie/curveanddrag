#pragma once
#include <rack.hpp>
#include <vector>
#include <array>
#include <atomic>
#include <cmath>
#include <random>

namespace CurveAndDrag {

enum WowFlutterWaveform {
    SINE,
    TRIANGLE,
    RANDOM
};

/**
 * TapeDelayProcessor - Emulation of tape delay characteristics
 * 
 * Provides wow & flutter, tape saturation, head bump EQ, and tape noise
 * to enhance the delay lines with vintage tape character
 */
class TapeDelayProcessor {
public:
    TapeDelayProcessor() {
        // Initialize with default values
        sampleRate = 44100.f;
        tapeModeEnabled = false;
        
        // Wow & Flutter
        wowRate = 0.3f;
        wowDepth = 0.1f;
        flutterRate = 2.7f;
        flutterDepth = 0.05f;
        wowPhase = 0.f;
        flutterPhase = 0.f;
        wowWaveform = SINE;
        flutterWaveform = SINE;
        
        // Tape Saturation
        saturationAmount = 0.5f;
        
        // Head Bump EQ
        bumpFrequency = 90.f;
        bumpGain = 1.5f;
        bumpQ = 1.2f;
        rolloffFreq = 10000.f;
        rolloffResonance = 0.7f;
        
        // Tape Noise
        noiseEnabled = false;
        noiseAmount = 0.01f;
        humFrequency = 60.f;
        humPhase = 0.f;
        
        // Initialize filters
        initializeFilters();
        
        // Initialize random generator
        rng.seed(std::random_device{}());
    }
    
    /**
     * Configure the processor with new sample rate
     */
    void configure(float newSampleRate) {
        if (sampleRate != newSampleRate) {
            sampleRate = newSampleRate;
            initializeFilters();
        }
    }
    
    /**
     * Reset all internal state
     */
    void reset() {
        wowPhase = 0.f;
        flutterPhase = 0.f;
        humPhase = 0.f;
        
        // Reset filters
        for (int i = 0; i < 2; i++) {
            bumpFilter[i].reset();
            rolloffFilter[i].reset();
            preEmphasisFilter[i].reset();
            deEmphasisFilter[i].reset();
        }
    }
    
    /**
     * Enable/disable tape mode
     */
    void setTapeMode(bool enabled) {
        tapeModeEnabled = enabled;
    }
    
    /**
     * Configure wow and flutter parameters
     */
    void setWowFlutter(float wowRateHz, float wowAmount, float flutterRateHz, float flutterAmount, WowFlutterWaveform wowType, WowFlutterWaveform flutterType) {
        wowRate = wowRateHz;
        wowDepth = wowAmount;
        flutterRate = flutterRateHz;
        flutterDepth = flutterAmount;
        wowWaveform = wowType;
        flutterWaveform = flutterType;
    }
    
    /**
     * Configure tape saturation amount
     */
    void setSaturation(float amount) {
        saturationAmount = amount;
    }
    
    /**
     * Configure head bump EQ
     */
    void setHeadBump(float frequency, float gain, float q) {
        bumpFrequency = frequency;
        bumpGain = gain;
        bumpQ = q;
        updateBumpFilter();
    }
    
    /**
     * Configure high-frequency rolloff
     */
    void setRolloff(float frequency, float resonance) {
        rolloffFreq = frequency;
        rolloffResonance = resonance;
        updateRolloffFilter();
    }
    
    /**
     * Enable/disable noise generation and set amount
     */
    void setNoiseParameters(bool enabled, float amount) {
        noiseEnabled = enabled;
        noiseAmount = amount;
    }
    
    /**
     * Calculate wow and flutter modulation value
     * 
     * @return Modulation value to apply to delay time (1.0 = no modulation)
     */
    float applyWowFlutter() {
        if (!tapeModeEnabled || (wowDepth < 0.001f && flutterDepth < 0.001f)) {
            return 1.0f;
        }
        
        // Update wow LFO phase
        wowPhase += wowRate / sampleRate;
        if (wowPhase >= 1.0f) {
            wowPhase -= 1.0f;
        }
        
        // Update flutter LFO phase
        flutterPhase += flutterRate / sampleRate;
        if (flutterPhase >= 1.0f) {
            flutterPhase -= 1.0f;
        }
        
        // Calculate wow modulation
        float wowMod = 0.0f;
        switch (wowWaveform) {
            case SINE:
                wowMod = std::sin(2.0f * M_PI * wowPhase);
                break;
            case TRIANGLE:
                // Triangle wave
                wowMod = 2.0f * std::abs(2.0f * (wowPhase - std::floor(wowPhase + 0.5f))) - 1.0f;
                break;
            case RANDOM:
                // Random walk with smoothing
                if (wowPhase < 0.01f || wowPhase > 0.99f) {
                    wowRandomTarget = randomUniform(-1.0f, 1.0f);
                }
                wowMod = wowMod * 0.99f + wowRandomTarget * 0.01f;
                break;
        }
        
        // Calculate flutter modulation
        float flutterMod = 0.0f;
        switch (flutterWaveform) {
            case SINE:
                flutterMod = std::sin(2.0f * M_PI * flutterPhase);
                break;
            case TRIANGLE:
                // Triangle wave
                flutterMod = 2.0f * std::abs(2.0f * (flutterPhase - std::floor(flutterPhase + 0.5f))) - 1.0f;
                break;
            case RANDOM:
                // Random fluctuation for flutter (more erratic)
                if (std::fmod(flutterPhase * 100.0f, 1.0f) < 0.1f) {
                    flutterRandomValue = randomUniform(-1.0f, 1.0f);
                }
                flutterMod = flutterRandomValue;
                break;
        }
        
        // Add some natural jitter/randomness to make it less predictable
        float jitter = randomUniform(-0.05f, 0.05f) * (wowDepth + flutterDepth);
        
        // Combine wow and flutter with jitter
        float totalMod = 1.0f + wowMod * wowDepth + flutterMod * flutterDepth + jitter;
        
        // Ensure we don't go negative or too extreme
        return std::max(0.8f, std::min(totalMod, 1.2f));
    }
    
    /**
     * Apply tape saturation to the signal
     * 
     * @param input Input sample
     * @return Saturated sample
     */
    float saturateSignal(float input) {
        if (!tapeModeEnabled || saturationAmount < 0.001f) {
            return input;
        }
        
        // Drive amount (increases with saturation)
        float drive = 1.0f + saturationAmount * 5.0f;
        float wetSignal = input * drive;
        
        // Soft clipping using tanh (hyperbolic tangent)
        // More extreme at higher saturation values
        wetSignal = std::tanh(wetSignal);
        
        // Blend between dry and saturated signal based on saturation amount
        return input * (1.0f - saturationAmount) + wetSignal * saturationAmount;
    }
    
    /**
     * Apply head bump EQ and high-frequency rolloff to signal
     * 
     * @param input Input sample
     * @param channel Channel index (0=left, 1=right)
     * @return Processed sample
     */
    float applyHeadBumpEQ(float input, int channel) {
        if (!tapeModeEnabled) {
            return input;
        }
        
        // Apply pre-emphasis (boosts highs before processing)
        float preEmph = preEmphasisFilter[channel].process(input);
        
        // Apply bass bump (centered around delay resonance frequency)
        float bumped = bumpFilter[channel].process(preEmph);
        
        // Apply high-frequency rolloff
        float rolled = rolloffFilter[channel].process(bumped);
        
        // Apply de-emphasis (cuts highs after processing)
        return deEmphasisFilter[channel].process(rolled);
    }
    
    /**
     * Generate and add tape noise and mechanical artifacts
     * 
     * @return Noise sample
     */
    float injectTapeNoise() {
        if (!tapeModeEnabled || !noiseEnabled) {
            return 0.0f;
        }
        
        // Generate pink noise component
        float whiteNoise = randomUniform(-1.0f, 1.0f);
        float pinkNoise = pinkNoiseFilter.process(whiteNoise);
        
        // Generate hum component (fundamental + harmonics)
        humPhase += humFrequency / sampleRate;
        if (humPhase >= 1.0f) {
            humPhase -= 1.0f;
        }
        
        float hum = 0.0f;
        hum += 0.7f * std::sin(2.0f * M_PI * humPhase); // Fundamental
        hum += 0.2f * std::sin(4.0f * M_PI * humPhase); // 2nd harmonic
        hum += 0.1f * std::sin(6.0f * M_PI * humPhase); // 3rd harmonic
        
        // Mix noise and hum
        float noise = pinkNoise * 0.7f + hum * 0.3f;
        
        // Apply gentle amplitude modulation for more realism
        float modulation = 0.9f + 0.1f * std::sin(2.0f * M_PI * wowPhase * 1.33f);
        
        return noise * noiseAmount * modulation;
    }

private:
    float sampleRate;
    std::atomic<bool> tapeModeEnabled;
    
    // Wow & Flutter
    float wowRate;
    float wowDepth;
    float flutterRate;
    float flutterDepth;
    float wowPhase;
    float flutterPhase;
    WowFlutterWaveform wowWaveform;
    WowFlutterWaveform flutterWaveform;
    float wowRandomTarget = 0.0f;
    float flutterRandomValue = 0.0f;
    
    // Tape Saturation
    float saturationAmount;
    
    // Head Bump EQ
    float bumpFrequency;
    float bumpGain;
    float bumpQ;
    float rolloffFreq;
    float rolloffResonance;
    std::array<rack::dsp::BiquadFilter, 2> bumpFilter;
    std::array<rack::dsp::BiquadFilter, 2> rolloffFilter;
    std::array<rack::dsp::BiquadFilter, 2> preEmphasisFilter;
    std::array<rack::dsp::BiquadFilter, 2> deEmphasisFilter;
    
    // Tape Noise
    bool noiseEnabled;
    float noiseAmount;
    float humFrequency;
    float humPhase;
    
    // Pink noise filter
    struct PinkNoiseFilter {
        float b0, b1, b2, b3, b4, b5, b6;
        
        PinkNoiseFilter() : b0(0.f), b1(0.f), b2(0.f), b3(0.f), b4(0.f), b5(0.f), b6(0.f) {}
        
        float process(float white) {
            b0 = 0.99886f * b0 + white * 0.0555179f;
            b1 = 0.99332f * b1 + white * 0.0750759f;
            b2 = 0.96900f * b2 + white * 0.1538520f;
            b3 = 0.86650f * b3 + white * 0.3104856f;
            b4 = 0.55000f * b4 + white * 0.5329522f;
            b5 = -0.7616f * b5 - white * 0.0168980f;
            
            return b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
        }
    } pinkNoiseFilter;
    
    // Random generator
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist{-1.0f, 1.0f};
    
    /**
     * Generate random uniform value
     */
    float randomUniform(float min, float max) {
        return min + dist(rng) * (max - min);
    }
    
    /**
     * Initialize all filters
     */
    void initializeFilters() {
        // Initialize head bump filters
        updateBumpFilter();
        
        // Initialize rolloff filters
        updateRolloffFilter();
        
        // Initialize pre-emphasis filter (boost highs before processing)
        for (int i = 0; i < 2; i++) {
            preEmphasisFilter[i].setHighShelf(sampleRate, 2000.0f, 0.7f, 2.0f);
            preEmphasisFilter[i].reset();
        }
        
        // Initialize de-emphasis filter (cut highs after processing)
        for (int i = 0; i < 2; i++) {
            deEmphasisFilter[i].setHighShelf(sampleRate, 2000.0f, 0.7f, 0.5f);
            deEmphasisFilter[i].reset();
        }
    }
    
    /**
     * Update the head bump filter parameters
     */
    void updateBumpFilter() {
        for (int i = 0; i < 2; i++) {
            bumpFilter[i].setPeakEQ(sampleRate, bumpFrequency, bumpQ, bumpGain);
        }
    }
    
    /**
     * Update the high-frequency rolloff filter parameters
     */
    void updateRolloffFilter() {
        for (int i = 0; i < 2; i++) {
            rolloffFilter[i].setLowpass(sampleRate, rolloffFreq, rolloffResonance);
        }
    }
};

} // namespace CurveAndDrag
