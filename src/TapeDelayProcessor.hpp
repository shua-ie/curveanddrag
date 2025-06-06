#pragma once
#include <rack.hpp>
#include <vector>
#include <array>
#include <atomic>
#include <cmath>
#include <random>

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
        // CRITICAL FIX: Ensure buffer is not empty and delay time is set
        if (buffer.empty() || delayTime <= 0.0f) {
            return 0.0f;
        }
        
        // Calculate read position with modulation
        float delayInSamples = delayTime * sampleRate / 1000.0f * modulation;
        int delaySamples = static_cast<int>(delayInSamples);
        float fraction = delayInSamples - delaySamples;
        
        // CRITICAL FIX: Ensure we don't read beyond buffer boundaries
        delaySamples = clamp(delaySamples, 1, static_cast<int>(buffer.size()) - 2);
        
        // Calculate read positions with bounds checking
        int pos1 = (writePos - delaySamples - 1 + buffer.size()) % buffer.size();
        int pos2 = (writePos - delaySamples + buffer.size()) % buffer.size();
        
        // CRITICAL FIX: Ensure positions are valid
        pos1 = clamp(pos1, 0, static_cast<int>(buffer.size()) - 1);
        pos2 = clamp(pos2, 0, static_cast<int>(buffer.size()) - 1);
        
        // Linear interpolation for smooth delay modulation
        float sample1 = buffer[pos1];
        float sample2 = buffer[pos2];
        
        return sample1 * (1.0f - fraction) + sample2 * fraction;
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
        tapeModeEnabled = enabled;
        
        // CRITICAL FIX: When enabling tape mode, ensure heads are properly initialized
        if (enabled) {
            // Initialize tape heads with safe default delay times
            for (int ch = 0; ch < 2; ch++) {
                recordHeads[ch].configure(sampleRate);
                for (int head = 0; head < 4; head++) {
                    playHeads[ch][head].configure(sampleRate);
                    // CRITICAL FIX: Set minimum delay time to prevent empty buffer reads
                    float headDelayTime = std::max(50.0f + head * 50.0f, 10.0f); // Minimum 10ms delay
                    playHeads[ch][head].setDelayTime(headDelayTime);
                }
            }
            // Initialize filters to prevent startup clicks
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
        if (agingAmount > 0.01f) {
            processed = applyAgingEffects(processed, channel);
        }
        
        // STEP 7: Apply instability effects
        if (instabilityAmount > 0.01f) {
            processed = applyInstabilityEffects(processed, channel);
        }
        
        // STEP 8: Add tape noise
        processed += injectTapeNoise();
        
        // STEP 9: Apply de-emphasis EQ
        processed = deEmphasisFilter[channel].process(processed);
        
        // STEP 10: Apply stereo decorrelation for channel separation
        processed = applyStereoDecorelation(processed, channel);
        
        // CRITICAL FIX: Final safety clamp and NaN check
        if (!std::isfinite(processed)) {
            processed = input * 0.7f; // Ultimate fallback to dry signal
        }
        
        // Soft limiting to prevent clipping
        processed = std::tanh(processed * 0.8f) / 0.8f;
        
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
        
        // Playback heads: Read from different positions based on head configuration
        switch (headConfiguration) {
            case 0: // Single head
                output = playHeads[channel][0].readFromTape(modulation);
                break;
                
            case 1: // Dual heads
                output = playHeads[channel][0].readFromTape(modulation) * 0.7f +
                        playHeads[channel][1].readFromTape(modulation * 1.05f) * 0.3f;
                break;
                
            case 2: // Triple heads
                output = playHeads[channel][0].readFromTape(modulation) * 0.5f +
                        playHeads[channel][1].readFromTape(modulation * 1.03f) * 0.3f +
                        playHeads[channel][2].readFromTape(modulation * 1.07f) * 0.2f;
                break;
                
            case 3: // Quad heads (full multi-head configuration)
                output = playHeads[channel][0].readFromTape(modulation) * 0.4f +
                        playHeads[channel][1].readFromTape(modulation * 1.02f) * 0.3f +
                        playHeads[channel][2].readFromTape(modulation * 1.05f) * 0.2f +
                        playHeads[channel][3].readFromTape(modulation * 1.08f) * 0.1f;
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
        // High frequency loss due to tape aging
        float cutoff = 1.0f - agingAmount * 0.3f;
        agingLowpass[channel] += (input - agingLowpass[channel]) * cutoff;
        
        // Slight modulation and warping
        float agingMod = 1.0f + std::sin(wowPhase * 13.7f) * agingAmount * 0.02f;
        
        // Add some random dropouts for very aged tape
        if (agingAmount > 0.7f && randomUniform(0.0f, 1.0f) < agingAmount * 0.0001f) {
            agingMod *= 0.3f; // Random dropout
        }
        
        // Blend aged signal
        float aged = agingLowpass[channel] * agingMod;
        return input * (1.0f - agingAmount * 0.4f) + aged * agingAmount * 0.4f;
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
        
        // ===== CRITICAL FIX: Musical noise scaling to prevent audio destruction =====
        
        // Generate pink noise component with proper scaling
        float whiteNoise = randomUniform(-1.0f, 1.0f);
        float pinkNoise = pinkNoiseFilter.process(whiteNoise);
        
        // ===== CRITICAL FIX: Scale pink noise to musical levels =====
        // Pink noise can be quite loud, so scale it down significantly
        pinkNoise *= 0.1f; // Reduce to 10% for musical use
        
        // Generate hum component (fundamental + harmonics)
        humPhase += humFrequency / sampleRate;
        if (humPhase >= 1.0f) {
            humPhase -= 1.0f;
        }
        
        float hum = 0.0f;
        hum += 0.7f * std::sin(2.0f * M_PI * humPhase); // Fundamental
        hum += 0.2f * std::sin(4.0f * M_PI * humPhase); // 2nd harmonic
        hum += 0.1f * std::sin(6.0f * M_PI * humPhase); // 3rd harmonic
        
        // ===== CRITICAL FIX: Scale hum to musical levels =====
        hum *= 0.05f; // Reduce hum amplitude significantly
        
        // Mix noise and hum with proper weighting
        float noise = pinkNoise * 0.8f + hum * 0.2f;
        
        // Apply gentle amplitude modulation for more realism
        float modulation = 0.95f + 0.05f * std::sin(2.0f * M_PI * wowPhase * 1.33f);
        
        // ===== CRITICAL FIX: Apply exponential curve to noise amount for better control =====
        // Square the noise amount for better low-level control
        float scaledNoiseAmount = noiseAmount * noiseAmount;
        
        // ===== CRITICAL FIX: Final safety clamp and musical scaling =====
        float finalNoise = noise * scaledNoiseAmount * modulation;
        
        // Clamp to ensure noise never overwhelms the signal
        finalNoise = clamp(finalNoise, -0.02f, 0.02f); // ±2% maximum noise level
        
        return finalNoise;
    }

    // Multi-head delay system (public for external access)
    std::array<TapeHead, 2> recordHeads;           // Record heads for L/R channels
    std::array<std::array<TapeHead, 4>, 2> playHeads; // Up to 4 playback heads per channel

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
    
    // Tape aging and instability
    float agingAmount = 0.0f;
    float instabilityAmount = 0.0f;
    int headConfiguration = 0;
    
    // Aging and instability state
    std::array<float, 2> agingLowpass = {0.0f, 0.0f};
    std::array<float, 2> instabilityPhase = {0.0f, 0.0f};
    
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
