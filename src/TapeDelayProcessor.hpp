#pragma once
#include <rack.hpp>
#include <vector>
#include <array>
#include <atomic>
#include <cmath>
#include <random>
#include "FeedbackProcessor.hpp"

namespace CurveAndDrag {

// Utility function for clamping values
template<typename T>
inline T clamp(T value, T min, T max) {
    return (value < min) ? min : (value > max) ? max : value;
}

enum WowFlutterWaveform {
    SINE,
    TRIANGLE,
    RANDOM
};

/**
 * Per-head pitch shifting and filter state
 *
 * Each tape playback head can independently:
 * - Pitch shift ±24 semitones + ±99 cents
 * - Apply SVF filter (LP/HP/BP/Notch) with cutoff and Q
 * - Set individual level and pan
 */
struct PerHeadDSP {
    // Pitch parameters
    float pitchSemitones = 0.0f;  // ±24 semitones
    float pitchCents = 0.0f;       // ±99 fine tune cents
    float level = 1.0f;            // 0-1 head level
    float pan = 0.5f;              // 0=left, 0.5=center, 1=right

    // SVF filter per head
    SVFilter filter;
    SVFilter::Mode filterMode = SVFilter::OFF;
    float filterCutoff = 8000.0f;
    float filterQ = 0.0f;

    // Pitch shifting state — 32768-sample buffer with cubic Hermite interpolation
    static constexpr int PITCH_BUF_SIZE = 32768;
    static constexpr int PITCH_BUF_MASK = PITCH_BUF_SIZE - 1;
    float readPhase = 0.0f;
    std::array<float, PITCH_BUF_SIZE> pitchBuffer = {};
    int pitchWritePos = 0;

    // Anti-alias one-pole LP state for pitch-up
    float aaLpState = 0.0f;

    // Parameter smoothing
    float smoothedRatio = 1.0f;

    void reset() {
        filter.reset();
        pitchBuffer.fill(0.0f);
        pitchWritePos = 0;
        // Offset readPhase behind writePos to prevent reading ahead
        readPhase = static_cast<float>(PITCH_BUF_SIZE / 2);
        aaLpState = 0.0f;
        smoothedRatio = 1.0f;
    }

    /**
     * Process: pitch shift + filter a single head's output
     */
    float process(float input, float sampleRate) {
        float processed = input;

        // Apply pitch shifting if non-zero
        float totalCents = pitchSemitones * 100.0f + pitchCents;
        if (std::abs(totalCents) > 1.0f) {
            float targetRatio = std::pow(2.0f, totalCents / 1200.0f);
            targetRatio = clamp(targetRatio, 0.25f, 4.0f);

            // Smooth ratio changes to prevent zipper noise (10ms ramp)
            float smoothCoeff = 1.0f - std::exp(-1.0f / (sampleRate * 0.01f));
            smoothedRatio += (targetRatio - smoothedRatio) * smoothCoeff;

            // Anti-alias pre-filter for pitch-up (one-pole LP)
            float writeInput = input;
            if (smoothedRatio > 1.01f) {
                float cutoff = 1.0f / smoothedRatio;
                aaLpState += (writeInput - aaLpState) * cutoff;
                writeInput = aaLpState;
            }

            // Write to pitch buffer
            pitchBuffer[pitchWritePos] = writeInput;
            pitchWritePos = (pitchWritePos + 1) & PITCH_BUF_MASK;

            // Read at shifted rate with cubic Hermite interpolation
            readPhase += smoothedRatio;
            if (readPhase >= static_cast<float>(PITCH_BUF_SIZE)) readPhase -= static_cast<float>(PITCH_BUF_SIZE);
            if (readPhase < 0.0f) readPhase += static_cast<float>(PITCH_BUF_SIZE);

            int idx1 = static_cast<int>(readPhase) & PITCH_BUF_MASK;
            float frac = readPhase - std::floor(readPhase);
            float p0 = pitchBuffer[(idx1 - 1 + PITCH_BUF_SIZE) & PITCH_BUF_MASK];
            float p1 = pitchBuffer[idx1];
            float p2 = pitchBuffer[(idx1 + 1) & PITCH_BUF_MASK];
            float p3 = pitchBuffer[(idx1 + 2) & PITCH_BUF_MASK];

            // Hermite interpolation
            float c0 = p1;
            float c1 = 0.5f * (p2 - p0);
            float c2 = p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3;
            float c3 = 0.5f * (p3 - p0) + 1.5f * (p1 - p2);
            processed = ((c3 * frac + c2) * frac + c1) * frac + c0;

            // Gain compensation
            float gainComp = 1.0f / std::sqrt(std::abs(smoothedRatio));
            gainComp = clamp(gainComp, 0.5f, 2.0f);
            processed *= gainComp;
        }

        // Apply SVF filter
        if (filterMode != SVFilter::OFF) {
            processed = filter.processMode(processed, filterMode, filterCutoff, filterQ, sampleRate);
        }

        // Apply level
        processed *= level;

        return processed;
    }

    /**
     * Get stereo pan gains
     */
    void getPanGains(float& leftGain, float& rightGain) const {
        // Equal-power pan law
        leftGain = std::cos(pan * M_PI * 0.5f);
        rightGain = std::sin(pan * M_PI * 0.5f);
    }
};

/**
 * Tape head structure for multi-head delay system
 */
struct TapeHead {
    std::vector<float> buffer;
    int writePos = 0;
    int readPos = 0;
    float delayTime = 0.0f;
    float sampleRate = 44100.0f;
    
    void configure(float sr, float maxDelayMs = 2000.0f) {
        sampleRate = sr;
        int bufferSize = static_cast<int>(maxDelayMs * sr / 1000.0f) + 1;
        buffer.resize(bufferSize, 0.0f);
        writePos = 0;
        readPos = 0;
    }
    
    void writeToTape(float sample) {
        buffer[writePos] = sample;
        writePos = (writePos + 1) % buffer.size();
    }
    
    float readFromTape(float modulation = 1.0f) {
        if (buffer.empty() || delayTime <= 0.0f) {
            return 0.0f;
        }

        float modulatedDelayTime = delayTime * modulation;
        float delayInSamples = modulatedDelayTime * sampleRate / 1000.0f;
        delayInSamples = clamp(delayInSamples, 1.0f, static_cast<float>(buffer.size()) - 4.0f);

        int bufSize = static_cast<int>(buffer.size());
        int idx = static_cast<int>(delayInSamples);
        float frac = delayInSamples - static_cast<float>(idx);

        // 4-point cubic Hermite interpolation
        float p0 = buffer[(writePos - idx - 1 + bufSize) % bufSize];
        float p1 = buffer[(writePos - idx + bufSize) % bufSize];
        float p2 = buffer[(writePos - idx + 1 + bufSize) % bufSize];
        float p3 = buffer[(writePos - idx + 2 + bufSize) % bufSize];

        float c0 = p1;
        float c1 = 0.5f * (p2 - p0);
        float c2 = p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3;
        float c3 = 0.5f * (p3 - p0) + 1.5f * (p1 - p2);
        return ((c3 * frac + c2) * frac + c1) * frac + c0;
    }
    
    void setDelayTime(float timeMs) {
        delayTime = clamp(timeMs, 1.0f, 2000.0f);
    }
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
        
        // Tape aging and instability
        agingAmount = 0.0f;
        instabilityAmount = 0.0f;
        headConfiguration = 0;
        
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
            
            // Configure tape heads
            for (int ch = 0; ch < 2; ch++) {
                recordHeads[ch].configure(sampleRate);
                for (int head = 0; head < 4; head++) {
                    playHeads[ch][head].configure(sampleRate);
                    // Set different delay times for multi-head effect
                    playHeads[ch][head].setDelayTime(100.0f + head * 50.0f); // 100ms, 150ms, 200ms, 250ms
                }
            }
        }
    }
    
    /**
     * Reset all internal state
     */
    void reset() {
        wowPhase = 0.f;
        flutterPhase = 0.f;
        humPhase = 0.f;
        
        // Reset aging and instability state
        for (int ch = 0; ch < 2; ch++) {
            agingLowpass[ch] = 0.0f;
            instabilityPhase[ch] = 0.0f;
        }
        
        // Reset tape heads
        for (int ch = 0; ch < 2; ch++) {
            recordHeads[ch].configure(sampleRate);
            for (int head = 0; head < 4; head++) {
                playHeads[ch][head].configure(sampleRate);
                playHeads[ch][head].setDelayTime(100.0f + head * 50.0f);
            }
        }
        
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
        if (tapeModeEnabled == enabled) return; // Only act on state change
        tapeModeEnabled = enabled;

        if (enabled) {
            // Initialize tape heads with safe default delay times (only on transition)
            for (int ch = 0; ch < 2; ch++) {
                recordHeads[ch].configure(sampleRate);
                for (int head = 0; head < 4; head++) {
                    playHeads[ch][head].configure(sampleRate);
                    float headDelayTime = std::max(50.0f + head * 50.0f, 10.0f);
                    playHeads[ch][head].setDelayTime(headDelayTime);
                }
            }
            initializeFilters();
        }
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
        // Rate-limit filter coefficient updates (every 32 samples)
        filterUpdateCounter++;
        if (filterUpdateCounter >= 32) {
            updateBumpFilter();
            updateRolloffFilter();
            filterUpdateCounter = 0;
        }
    }

    /**
     * Configure high-frequency rolloff
     */
    void setRolloff(float frequency, float resonance) {
        rolloffFreq = frequency;
        rolloffResonance = resonance;
        // Coefficients updated via rate-limited setHeadBump
    }
    
    /**
     * Enable/disable noise generation and set amount
     */
    void setNoiseParameters(bool enabled, float amount) {
        noiseEnabled = enabled;
        noiseAmount = amount;
    }
    
    /**
     * Set wow parameters
     */
    void setWowParameters(float rate, float depth) {
        wowRate = rate;
        wowDepth = depth;
    }
    
    /**
     * Set flutter parameters
     */
    void setFlutterParameters(float rate, float depth) {
        flutterRate = rate;
        flutterDepth = depth;
    }
    
    /**
     * Set flutter depth only
     */
    void setFlutterDepth(float depth) {
        flutterDepth = depth;
    }
    
    /**
     * Set tape aging amount
     */
    void setAging(float amount) {
        agingAmount = amount;
    }
    
    /**
     * Set tape instability amount
     */
    void setInstability(float amount) {
        instabilityAmount = amount;
    }
    
    /**
     * Set head configuration (0=single, 1=dual, 2=triple, 3=quad)
     */
    void setHeadConfiguration(int config) {
        headConfiguration = clamp(config, 0, 3);
        
        // ===== CRITICAL FIX: Immediately update head delay times when configuration changes =====
        // This ensures users hear audible differences between head modes
        for (int ch = 0; ch < 2; ch++) {
            switch (headConfiguration) {
                case 0: // Single head - 120ms delay
                    playHeads[ch][0].setDelayTime(120.0f);
                    playHeads[ch][1].setDelayTime(0.0f);   // Disable other heads
                    playHeads[ch][2].setDelayTime(0.0f);
                    playHeads[ch][3].setDelayTime(0.0f);
                    break;
                    
                case 1: // Dual heads - 100ms and 170ms for stereo width
                    playHeads[ch][0].setDelayTime(100.0f);
                    playHeads[ch][1].setDelayTime(170.0f);
                    playHeads[ch][2].setDelayTime(0.0f);   // Disable unused heads
                    playHeads[ch][3].setDelayTime(0.0f);
                    break;
                    
                case 2: // Triple heads - 80ms, 140ms, 200ms for rich complexity
                    playHeads[ch][0].setDelayTime(80.0f);
                    playHeads[ch][1].setDelayTime(140.0f);
                    playHeads[ch][2].setDelayTime(200.0f);
                    playHeads[ch][3].setDelayTime(0.0f);   // Disable unused head
                    break;
                    
                case 3: // Quad heads - 70ms, 120ms, 180ms, 250ms for maximum complexity
                    playHeads[ch][0].setDelayTime(70.0f);
                    playHeads[ch][1].setDelayTime(120.0f);
                    playHeads[ch][2].setDelayTime(180.0f);
                    playHeads[ch][3].setDelayTime(250.0f);
                    break;
            }
        }
    }
    
    /**
     * Set noise amount only
     */
    void setNoiseAmount(float amount) {
        noiseAmount = amount;
    }
    
    /**
     * Process a single audio sample through tape delay
     * 
     * @param input Input sample
     * @param channel Channel index (0=left, 1=right)
     * @return Processed sample
     */
    float process(float input, int channel) {
        if (!tapeModeEnabled) {
            return input;
        }
        
        // CRITICAL FIX: Safety check for valid channel
        if (channel < 0 || channel > 1) {
            return input;
        }
        
        // CRITICAL FIX: Ensure input is not NaN or infinite
        if (!std::isfinite(input)) {
            return 0.0f;
        }
        
        // TAPE PROCESSING FLOW: Pre-EQ → Wow/Flutter → Saturation → Head Bump → Aging → Noise → Stereo Sum
        
        // STEP 1: Apply pre-emphasis EQ
        float processed = preEmphasisFilter[channel].process(input);
        
        // CRITICAL FIX: Check for NaN after filtering
        if (!std::isfinite(processed)) {
            processed = input;
        }
        
        // STEP 2: Apply wow and flutter modulation to delay time
        float modulationAmount = applyWowFlutter();
        
        // STEP 3: Apply tape saturation
        processed = saturateSignal(processed);
        
        // CRITICAL FIX: Check for NaN after saturation
        if (!std::isfinite(processed)) {
            processed = input * 0.5f; // Fallback to attenuated dry signal
        }
        
        // STEP 4: Multi-head delay processing
        float delayedSignal = processMultiHeadDelay(processed, channel, modulationAmount);
        
        // CRITICAL FIX: If delay processing fails, mix with dry signal
        if (!std::isfinite(delayedSignal) || std::abs(delayedSignal) < 1e-10f) {
            delayedSignal = processed * 0.8f; // Use processed dry signal as fallback
        }
        
        processed = delayedSignal;
        
        // STEP 5: Apply head bump EQ and high-frequency rolloff
        processed = applyHeadBumpEQ(processed, channel);
        
        // CRITICAL FIX: Check for NaN after EQ
        if (!std::isfinite(processed)) {
            processed = delayedSignal;
        }
        
        // STEP 6: Apply aging effects
        if (agingAmount > 0.001f) {
            processed = applyAgingEffects(processed, channel);
        }
        
        // STEP 7: Apply instability effects
        if (instabilityAmount > 0.001f) {
            processed = applyInstabilityEffects(processed, channel);
        }
        
        // STEP 8: Add tape noise
        processed += injectTapeNoise();
        
        // STEP 9: Apply de-emphasis EQ
        processed = deEmphasisFilter[channel].process(processed);
        
        // STEP 10: Apply stereo decorrelation for channel separation
        processed = applyStereoDecorelation(processed, channel);
        
        // DC blocking after nonlinear tape stages
        {
            float dcOut = processed - tapeDcBlockState[channel];
            tapeDcBlockState[channel] = processed - dcOut * 0.9999f;
            processed = dcOut;
        }

        // Final safety clamp and NaN check
        if (!std::isfinite(processed)) {
            processed = input * 0.7f; // Ultimate fallback to dry signal
        }

        // Gentle soft limiting — preserves dynamics
        if (std::abs(processed) > 1.0f) {
            processed = std::tanh(processed);
        }
        
        return processed;
    }
    
    /**
     * Process multi-head delay system
     * 
     * @param input Input sample
     * @param channel Channel index
     * @param modulation Wow/flutter modulation amount
     * @return Processed sample
     */
    float processMultiHeadDelay(float input, int channel, float modulation) {
        // Record head: Write to the tape buffer
        recordHeads[channel].writeToTape(input);
        
        float output = 0.0f;
        
        // ===== CRITICAL FIX: Completely Rewritten Head Configuration System =====
        switch (headConfiguration) {
            case 0: // Single head - Clean, focused sound
                {
                    // ===== CRITICAL FIX: Only read from active heads =====
                    if (playHeads[channel][0].delayTime > 0.0f) {
                        output = playHeads[channel][0].readFromTape(modulation);
                        output = headDSP[channel][0].process(output, sampleRate);

                        // Apply subtle EQ for single-head character (brighter)
                        float hpCoeff = 0.95f; // Light high-pass
                        singleHeadHighpass[channel] += (output - singleHeadHighpass[channel]) * hpCoeff;
                        output = output - singleHeadHighpass[channel] * 0.1f; // Slight high boost
                    }
                }
                break;
                
            case 1: // Dual heads - Stereo width and slight chorus
                {
                    // ===== CRITICAL FIX: Check both heads are active and apply stereo routing =====
                    float head1 = 0.0f, head2 = 0.0f;
                    
                    if (playHeads[channel][0].delayTime > 0.0f) {
                        head1 = playHeads[channel][0].readFromTape(modulation);
                        head1 = headDSP[channel][0].process(head1, sampleRate);
                    }
                    if (playHeads[channel][1].delayTime > 0.0f) {
                        head2 = playHeads[channel][1].readFromTape(modulation * 1.03f); // Slightly different rate
                        head2 = headDSP[channel][1].process(head2, sampleRate);
                    }

                    // ===== CRITICAL FIX: Proper stereo panning for dual heads =====
                    if (channel == 0) {
                        // Left channel: emphasize head 1, subtle head 2
                        output = head1 * 0.8f + head2 * 0.4f;
                    } else {
                        // Right channel: emphasize head 2, subtle head 1  
                        output = head1 * 0.4f + head2 * 0.8f;
                    }
                    
                    // Apply slight chorus effect between heads for movement
                    float chorusMix = 0.2f;
                    output = output * (1.0f - chorusMix) + (head1 - head2) * chorusMix;
                }
                break;
                
            case 2: // Triple heads - Rich harmonics and depth
                {
                    // ===== CRITICAL FIX: Sum all active heads with progressive modulation =====
                    float head1 = 0.0f, head2 = 0.0f, head3 = 0.0f;
                    
                    if (playHeads[channel][0].delayTime > 0.0f) {
                        head1 = playHeads[channel][0].readFromTape(modulation);
                        head1 = headDSP[channel][0].process(head1, sampleRate);
                    }
                    if (playHeads[channel][1].delayTime > 0.0f) {
                        head2 = playHeads[channel][1].readFromTape(modulation * 1.02f);
                        head2 = headDSP[channel][1].process(head2, sampleRate);
                    }
                    if (playHeads[channel][2].delayTime > 0.0f) {
                        head3 = playHeads[channel][2].readFromTape(modulation * 1.05f);
                        head3 = headDSP[channel][2].process(head3, sampleRate);
                    }

                    // Mix with weighted blend for richness
                    output = head1 * 0.5f + head2 * 0.3f + head3 * 0.2f;
                    
                    // Add harmonic interaction between heads
                    float harmonic = (head1 * head2 + head2 * head3) * 0.05f;
                    output += harmonic;
                    
                    // Apply mid-frequency emphasis for warmth
                    float midCoeff = 0.85f;
                    tripleHeadMidEQ[channel] += (output - tripleHeadMidEQ[channel]) * midCoeff;
                    output = output + tripleHeadMidEQ[channel] * 0.1f; // Mid boost
                }
                break;
                
            case 3: // Quad heads - Maximum complexity and vintage character
                {
                    // ===== CRITICAL FIX: Read from all active heads with distinct characteristics =====
                    float head1 = 0.0f, head2 = 0.0f, head3 = 0.0f, head4 = 0.0f;
                    
                    if (playHeads[channel][0].delayTime > 0.0f) {
                        head1 = playHeads[channel][0].readFromTape(modulation);           // Main head
                        head1 = headDSP[channel][0].process(head1, sampleRate);
                    }
                    if (playHeads[channel][1].delayTime > 0.0f) {
                        head2 = playHeads[channel][1].readFromTape(modulation * 1.015f);  // Slight detune
                        head2 = headDSP[channel][1].process(head2, sampleRate);
                    }
                    if (playHeads[channel][2].delayTime > 0.0f) {
                        head3 = playHeads[channel][2].readFromTape(modulation * 1.03f);   // More detune
                        head3 = headDSP[channel][2].process(head3, sampleRate);
                    }
                    if (playHeads[channel][3].delayTime > 0.0f) {
                        head4 = playHeads[channel][3].readFromTape(modulation * 1.045f);  // Maximum detune
                        head4 = headDSP[channel][3].process(head4, sampleRate);
                    }
                    
                    // Progressive mixing for complex texture
                    output = head1 * 0.4f + head2 * 0.25f + head3 * 0.2f + head4 * 0.15f;
                    
                    // Add inter-head modulation for vintage tape character
                    float intermod = (head1 * head3 - head2 * head4) * 0.03f;
                    output += intermod;
                    
                    // Apply complex EQ curve for vintage warmth
                    
                    // Low-pass for warmth
                    float lpCoeff = 0.75f;
                    quadHeadLowpass[channel] += (output - quadHeadLowpass[channel]) * lpCoeff;
                    
                    // Mid-boost for presence
                    float mbCoeff = 0.9f;
                    quadHeadMidboost[channel] += (output - quadHeadMidboost[channel]) * mbCoeff;
                    
                    // Combine EQ stages
                    output = quadHeadLowpass[channel] * 0.7f + (output + quadHeadMidboost[channel] * 0.08f) * 0.3f;
                    
                    // Add subtle saturation for tape character
                    output = std::tanh(output * 1.1f) / 1.1f;
                }
                break;
        }
        
        return output;
    }
    
    /**
     * Apply aging effects to simulate old tape
     * 
     * @param input Input sample
     * @param channel Channel index
     * @return Processed sample
     */
    float applyAgingEffects(float input, int channel) {
        // ===== CRITICAL FIX: Much more responsive aging effect =====
        
        // High frequency loss due to tape aging - exponential curve for better control
        float agingSquared = agingAmount * agingAmount; // Square for exponential response
        float cutoffFreq = 1.0f - agingSquared * 0.7f; // Much more aggressive high-frequency rolloff
        cutoffFreq = std::max(cutoffFreq, 0.1f); // Prevent total cutoff
        
        agingLowpass[channel] += (input - agingLowpass[channel]) * cutoffFreq;
        
        // ===== CRITICAL FIX: Add tape degradation artifacts =====
        // Slight modulation and warping becomes more noticeable
        float agingMod = 1.0f + std::sin(wowPhase * 13.7f) * agingAmount * 0.1f; // Increased from 0.02f
        
        // Add some random dropouts for aged tape - more frequent at higher aging
        if (agingAmount > 0.3f && randomUniform(0.0f, 1.0f) < agingAmount * 0.0005f) { // Increased dropout frequency
            agingMod *= 0.5f; // Less severe dropout than before
        }
        
        // ===== CRITICAL FIX: Add tape compression/limiting simulation =====
        float compressed = std::tanh(agingLowpass[channel] * agingMod * (1.0f + agingAmount * 0.5f));
        
        // ===== CRITICAL FIX: More aggressive blending for audible effect =====
        float wetAmount = agingAmount * 0.8f; // Increased from 0.4f
        return input * (1.0f - wetAmount) + compressed * wetAmount;
    }
    
    /**
     * Apply instability effects to simulate mechanical issues
     * 
     * @param input Input sample
     * @param channel Channel index
     * @return Processed sample
     */
    float applyInstabilityEffects(float input, int channel) {
        // ===== CRITICAL FIX: Exponential scaling for instability =====
        // Apply square curve to make low levels more subtle
        float scaledInstability = instabilityAmount * instabilityAmount;
        
        // Update instability phase for random variations
        instabilityPhase[channel] += scaledInstability * 0.005f + randomUniform(-0.0005f, 0.0005f); // Reduced rates
        if (instabilityPhase[channel] >= 1.0f) instabilityPhase[channel] -= 1.0f;
        
        // ===== CRITICAL FIX: Much more subtle level variations =====
        float levelMod = 1.0f + std::sin(instabilityPhase[channel] * 2.0f * M_PI) * scaledInstability * 0.05f; // Reduced from 0.15f
        
        // ===== CRITICAL FIX: Reduce random dropout frequency significantly =====
        if (randomUniform(0.0f, 1.0f) < scaledInstability * 0.0002f) { // Reduced from 0.002f
            levelMod *= 0.7f; // Less severe dropout (was 0.2f)
        }
        
        // ===== CRITICAL FIX: Much more subtle speed variations =====
        float speedVar = 1.0f + std::sin(instabilityPhase[channel] * 7.3f * M_PI) * scaledInstability * 0.01f; // Reduced from 0.05f
        
        return input * levelMod * speedVar;
    }
    
    /**
     * Apply stereo decorrelation for channel separation
     * 
     * @param input Input sample
     * @param channel Channel index
     * @return Decorrelated sample
     */
    float applyStereoDecorelation(float input, int channel) {
        // Apply different modulation per channel for stereo width
        float channelOffset = (channel == 0) ? 0.0f : 0.25f; // 90-degree phase offset for right channel
        float decorrelationPhase = wowPhase + flutterPhase * 0.7f + channelOffset;
        
        // Subtle decorrelation modulation
        float decorrelation = 1.0f + std::sin(decorrelationPhase * 2.0f * M_PI * 3.17f) * 0.02f;
        
        return input * decorrelation;
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
        switch (wowWaveform) {
            case SINE:
                wowSmoothedMod = std::sin(2.0f * M_PI * wowPhase);
                break;
            case TRIANGLE:
                wowSmoothedMod = 2.0f * std::abs(2.0f * (wowPhase - std::floor(wowPhase + 0.5f))) - 1.0f;
                break;
            case RANDOM:
                if (wowPhase < 0.01f || wowPhase > 0.99f) {
                    wowRandomTarget = randomUniform(-1.0f, 1.0f);
                }
                wowSmoothedMod = wowSmoothedMod * 0.99f + wowRandomTarget * 0.01f;
                break;
        }
        float wowMod = wowSmoothedMod;
        
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
        
        // 1/f noise component for wow (3 octaves of filtered noise)
        float wowWhite = randomUniform(-1.0f, 1.0f);
        pinkWowB0 = 0.99765f * pinkWowB0 + wowWhite * 0.0990460f;
        pinkWowB1 = 0.96300f * pinkWowB1 + wowWhite * 0.2965164f;
        pinkWowB2 = 0.57000f * pinkWowB2 + wowWhite * 1.0526913f;
        float wowPinkNoise = (pinkWowB0 + pinkWowB1 + pinkWowB2) * 0.15f;

        // Slow random walk for capstan irregularity
        wowRandomWalk += randomUniform(-0.0001f, 0.0001f);
        wowRandomWalk *= 0.9999f; // Decay toward zero
        float wowWithNoise = wowMod * 0.7f + wowPinkNoise * 0.2f + wowRandomWalk * 0.1f;

        // 1/f noise component for flutter
        float flutterWhite = randomUniform(-1.0f, 1.0f);
        pinkFlutterB0 = 0.99765f * pinkFlutterB0 + flutterWhite * 0.0990460f;
        pinkFlutterB1 = 0.96300f * pinkFlutterB1 + flutterWhite * 0.2965164f;
        pinkFlutterB2 = 0.57000f * pinkFlutterB2 + flutterWhite * 1.0526913f;
        float flutterPinkNoise = (pinkFlutterB0 + pinkFlutterB1 + pinkFlutterB2) * 0.15f;
        float flutterWithNoise = flutterMod * 0.7f + flutterPinkNoise * 0.3f;

        // Combine wow and flutter with 1/f noise
        float totalMod = 1.0f + wowWithNoise * wowDepth + flutterWithNoise * flutterDepth;

        // Ensure we don't go negative or too extreme
        return std::max(0.8f, std::min(totalMod, 1.2f));
    }
    
    /**
     * Apply tape saturation with asymmetric response and hysteresis
     *
     * @param input Input sample
     * @return Saturated sample
     */
    float saturateSignal(float input) {
        if (!tapeModeEnabled || saturationAmount < 0.001f) {
            return input;
        }

        // Logarithmic drive curve (like real tape)
        float drive = std::exp(saturationAmount * 2.0f);

        // Two-band frequency-dependent saturation
        // Low band (~300Hz split) saturates harder, high band stays cleaner
        float lpCoeff = 300.0f / sampleRate * 2.0f * static_cast<float>(M_PI);
        lpCoeff = clamp(lpCoeff, 0.001f, 0.5f);
        satLpState += (input - satLpState) * lpCoeff;
        float lowContent = satLpState;
        float highContent = input - lowContent;

        // Asymmetric saturation (tape saturates differently for +/-)
        float satLow = std::tanh(lowContent * drive * 1.5f);
        if (lowContent > 0.0f) satLow *= 1.02f; // Subtle asymmetry
        float satHigh = std::tanh(highContent * drive * 0.6f);

        float saturated = satLow + satHigh;

        // Magnetic hysteresis: current output depends on previous state
        float hystCoeff = 0.2f * saturationAmount;
        float hysteresis = saturated + (satHystState - saturated) * hystCoeff;
        satHystState = saturated;

        // Blend between dry and saturated signal
        return input * (1.0f - saturationAmount) + hysteresis * saturationAmount;
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

        // Apply bass bump (centered around delay resonance frequency)
        float bumped = bumpFilter[channel].process(input);

        // Apply high-frequency rolloff
        return rolloffFilter[channel].process(bumped);
    }
    
    /**
     * Inject tape noise into the signal
     * 
     * @return Noise sample to be added to the signal
     */
    float injectTapeNoise() {
        if (!noiseEnabled) {
            return 0.0f;
        }
        
        // Logarithmic scaling for natural noise level control
        // At full amount: ~25% signal level (realistic tape hiss)
        float scaledNoise = noiseAmount * 0.25f;

        if (scaledNoise < 0.0001f) {
            return 0.0f;
        }
        
        // Generate high-quality pink noise
        float whiteNoise = dist(rng);
        float pinkNoise = pinkNoiseFilter.process(whiteNoise);
        
        // ===== CRITICAL FIX: Multiple Noise Components =====
        
        // 1. Main tape hiss (pink noise)
        float tapeHiss = pinkNoise * scaledNoise;
        
        // 2. 60Hz hum with harmonics (120Hz, 180Hz)
        humPhase += humFrequency / sampleRate;
        if (humPhase >= 1.0f) humPhase -= 1.0f;
        float hum = (std::sin(2.0f * M_PI * humPhase)
                    + 0.5f * std::sin(4.0f * M_PI * humPhase)
                    + 0.25f * std::sin(6.0f * M_PI * humPhase)) * scaledNoise * 0.1f;
        
        // 3. High-frequency tape artifacts (occasional)
        float artifacts = 0.0f;
        noiseArtifactCounter++;
        if (noiseArtifactCounter > 22050 && randomUniform(0.0f, 1.0f) < scaledNoise * 0.2f) {
            artifacts = randomUniform(-1.0f, 1.0f) * scaledNoise * 0.5f;
            noiseArtifactCounter = 0;
        }
        
        // 4. Low-frequency rumble (DC offset simulation)
        noiseRumblePhase += 1.7f / sampleRate; // Very low frequency
        if (noiseRumblePhase >= 1.0f) noiseRumblePhase -= 1.0f;
        float rumble = std::sin(2.0f * M_PI * noiseRumblePhase) * scaledNoise * 0.05f;
        
        // ===== CRITICAL FIX: Combine all noise components additively =====
        float totalNoise = tapeHiss + hum + artifacts + rumble;
        
        // Soft-limit noise spikes
        totalNoise = std::tanh(totalNoise * 4.0f) * 0.25f;
        
        return totalNoise;
    }

    // Multi-head delay system (public for external access)
    std::array<TapeHead, 2> recordHeads;           // Record heads for L/R channels
    std::array<std::array<TapeHead, 4>, 2> playHeads; // Up to 4 playback heads per channel

    // Per-head DSP (pitch + filter per tap)
    std::array<std::array<PerHeadDSP, 4>, 2> headDSP; // Per-head pitch/filter for L/R x 4 heads

    /**
     * Configure per-head DSP parameters
     * @param channel 0=left, 1=right
     * @param head Head index 0-3
     * @param semitones Pitch shift in semitones (±24)
     * @param cents Fine tune in cents (±99)
     * @param filterMode SVFilter mode (OFF/LP/HP/BP/NOTCH)
     * @param cutoff Filter cutoff frequency
     * @param q Filter resonance (0-1)
     * @param level Head level (0-1)
     * @param pan Head pan (0=L, 0.5=C, 1=R)
     */
    void setHeadDSP(int channel, int head, float semitones, float cents,
                    SVFilter::Mode filterMode, float cutoff, float q,
                    float level, float pan) {
        if (channel < 0 || channel > 1 || head < 0 || head > 3) return;
        auto& dsp = headDSP[channel][head];
        dsp.pitchSemitones = clamp(semitones, -24.0f, 24.0f);
        dsp.pitchCents = clamp(cents, -99.0f, 99.0f);
        dsp.filterMode = filterMode;
        dsp.filterCutoff = clamp(cutoff, 20.0f, 20000.0f);
        dsp.filterQ = clamp(q, 0.0f, 1.0f);
        dsp.level = clamp(level, 0.0f, 1.0f);
        dsp.pan = clamp(pan, 0.0f, 1.0f);
    }

    /**
     * Process multi-head delay with per-head pitch shifting and filtering
     * Returns stereo pair (left, right) for proper per-head panning
     */
    void processMultiHeadStereo(float input, int channel, float modulation,
                                 float& outLeft, float& outRight) {
        recordHeads[channel].writeToTape(input);

        outLeft = 0.0f;
        outRight = 0.0f;

        int numHeads = headConfiguration + 1;
        for (int h = 0; h < numHeads; h++) {
            if (playHeads[channel][h].delayTime <= 0.0f) continue;

            // Read raw delayed signal from this head
            float headSignal = playHeads[channel][h].readFromTape(modulation);

            // Apply per-head DSP (pitch shift + filter + level)
            headSignal = headDSP[channel][h].process(headSignal, sampleRate);

            // Apply per-head panning
            float panL, panR;
            headDSP[channel][h].getPanGains(panL, panR);
            outLeft += headSignal * panL;
            outRight += headSignal * panR;
        }
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
    float wowSmoothedMod = 0.0f; // Persists across samples for random smoothing
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
    
    // Tape aging and instability
    float agingAmount = 0.0f;
    float instabilityAmount = 0.0f;
    int headConfiguration = 0;
    
    // Aging and instability state
    std::array<float, 2> agingLowpass = {0.0f, 0.0f};
    std::array<float, 2> instabilityPhase = {0.0f, 0.0f};

    // DC blocker state per channel (one-pole highpass ~5Hz)
    std::array<float, 2> tapeDcBlockState = {0.0f, 0.0f};

    // Tape saturation state
    float satLpState = 0.0f;   // Low-pass state for frequency-dependent saturation
    float satHystState = 0.0f; // Hysteresis state for magnetic remnance

    // 1/f noise state for wow/flutter realism
    float pinkWowB0 = 0.0f, pinkWowB1 = 0.0f, pinkWowB2 = 0.0f;
    float pinkFlutterB0 = 0.0f, pinkFlutterB1 = 0.0f, pinkFlutterB2 = 0.0f;
    float wowRandomWalk = 0.0f; // Slow random walk for capstan irregularity

    // Per-head EQ filter state (per-channel, NOT static to avoid cross-instance contamination)
    std::array<float, 2> singleHeadHighpass = {0.0f, 0.0f};
    std::array<float, 2> tripleHeadMidEQ = {0.0f, 0.0f};
    std::array<float, 2> quadHeadLowpass = {0.0f, 0.0f};
    std::array<float, 2> quadHeadMidboost = {0.0f, 0.0f};

    // Noise state (member variables, not static)
    int noiseArtifactCounter = 0;
    float noiseRumblePhase = 0.0f;

    // Filter coefficient rate-limiting
    int filterUpdateCounter = 0;
    
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
            preEmphasisFilter[i].setParameters(rack::dsp::BiquadFilter::HIGHSHELF, 2000.0f / sampleRate, 0.7f, 2.0f);
            preEmphasisFilter[i].reset();
        }
        
        // Initialize de-emphasis filter (cut highs after processing)
        for (int i = 0; i < 2; i++) {
            deEmphasisFilter[i].setParameters(rack::dsp::BiquadFilter::HIGHSHELF, 2000.0f / sampleRate, 0.7f, 0.5f);
            deEmphasisFilter[i].reset();
        }
    }
    
    /**
     * Update the head bump filter parameters
     */
    void updateBumpFilter() {
        for (int i = 0; i < 2; i++) {
            bumpFilter[i].setParameters(rack::dsp::BiquadFilter::PEAK, bumpFrequency / sampleRate, bumpQ, bumpGain);
        }
    }
    
    /**
     * Update the high-frequency rolloff filter parameters
     */
    void updateRolloffFilter() {
        for (int i = 0; i < 2; i++) {
            rolloffFilter[i].setParameters(rack::dsp::BiquadFilter::LOWPASS, rolloffFreq / sampleRate, rolloffResonance, 1.0f);
        }
    }
};

} // namespace CurveAndDrag
