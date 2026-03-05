#pragma once
#include <rack.hpp>
#include <cmath>
#include <random>
#include <array>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace CurveAndDrag {

/**
 * State-Variable Filter for per-tap filtering and feedback path EQ
 * Trapezoidal integrated SVF (Andrew Simper / Cytomic design)
 */
struct SVFilter {
    enum Mode { OFF = 0, LOWPASS, HIGHPASS, BANDPASS, NOTCH };

    float ic1eq = 0.0f, ic2eq = 0.0f;

    void reset() { ic1eq = 0.0f; ic2eq = 0.0f; }

    struct Output { float lp, hp, bp, notch; };

    Output process(float v0, float cutoff, float resonance, float sampleRate) {
        float g = std::tan(M_PI * std::min(cutoff / sampleRate, 0.49f));
        float k = 2.0f - 2.0f * resonance; // resonance 0..1 maps to k 2..0
        k = std::max(k, 0.01f);
        float a1 = 1.0f / (1.0f + g * (g + k));
        float a2 = g * a1;
        float a3 = g * a2;

        float v3 = v0 - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        Output out;
        out.lp = v2;
        out.bp = v1;
        out.hp = v0 - k * v1 - v2;
        out.notch = out.lp + out.hp;
        return out;
    }

    float processMode(float input, Mode mode, float cutoff, float q, float sr) {
        if (mode == OFF) return input;
        auto out = process(input, cutoff, q, sr);
        switch (mode) {
            case LOWPASS: return out.lp;
            case HIGHPASS: return out.hp;
            case BANDPASS: return out.bp;
            case NOTCH: return out.notch;
            default: return input;
        }
    }
};

/**
 * Simple one-pole envelope follower for ducking and envelope-driven pitch
 */
struct EnvelopeFollower {
    float envelope = 0.0f;

    void reset() { envelope = 0.0f; }

    float process(float input, float attackCoeff, float releaseCoeff) {
        float absInput = std::abs(input);
        if (absInput > envelope)
            envelope += (absInput - envelope) * attackCoeff;
        else
            envelope += (absInput - envelope) * releaseCoeff;
        return envelope;
    }
};

/**
 * Hilbert transform for frequency shifting (Ghost mode)
 * Uses allpass filter pairs with 90° phase difference
 */
struct HilbertTransform {
    // 4-stage allpass cascade for each branch (real and imaginary)
    // Coefficients for ~20Hz-20kHz bandwidth at 44.1kHz
    static constexpr int NUM_STAGES = 4;
    float realCoeffs[NUM_STAGES] = {0.6923878f, 0.9360654322959f, 0.9882295226860f, 0.9987488452737f};
    float imagCoeffs[NUM_STAGES] = {0.4021921162426f, 0.8561710882420f, 0.9722909545651f, 0.9952884791278f};

    float realState[NUM_STAGES][2] = {};
    float imagState[NUM_STAGES][2] = {};

    void reset() {
        for (int i = 0; i < NUM_STAGES; i++) {
            realState[i][0] = realState[i][1] = 0.0f;
            imagState[i][0] = imagState[i][1] = 0.0f;
        }
    }

    float allpass(float input, float coeff, float state[2]) {
        float output = coeff * (input - state[1]) + state[0];
        state[0] = input;
        state[1] = output;
        return output;
    }

    // Returns {real, imaginary} pair (90° phase shifted)
    void process(float input, float& outReal, float& outImag) {
        outReal = input;
        outImag = input;
        for (int i = 0; i < NUM_STAGES; i++) {
            outReal = allpass(outReal, realCoeffs[i], realState[i]);
            outImag = allpass(outImag, imagCoeffs[i], imagState[i]);
        }
    }
};

/**
 * Frequency Shifter using Hilbert transform + complex oscillator
 */
struct FrequencyShifter {
    HilbertTransform hilbert;
    float phase = 0.0f;

    void reset() {
        hilbert.reset();
        phase = 0.0f;
    }

    float process(float input, float shiftHz, float sampleRate) {
        if (std::abs(shiftHz) < 0.01f) return input;

        phase += shiftHz / sampleRate;
        if (phase > 1.0f) phase -= 1.0f;
        if (phase < -1.0f) phase += 1.0f;

        float cosPhase = std::cos(2.0f * M_PI * phase);
        float sinPhase = std::sin(2.0f * M_PI * phase);

        float hilbertReal, hilbertImag;
        hilbert.process(input, hilbertReal, hilbertImag);

        // Single-sideband modulation (upper sideband)
        return hilbertReal * cosPhase - hilbertImag * sinPhase;
    }
};

/**
 * FeedbackProcessor - Handles all processing inside the feedback loop
 *
 * This is the core of the shimmer/cascade architecture.
 * Signal flow: Delayed signal → Pitch Shift → LP/HP Filter → Saturation → Compression → Output
 */
class FeedbackProcessor {
public:
    // Per-channel state
    struct ChannelState {
        SVFilter lpFilter;
        SVFilter hpFilter;
        SVFilter tiltFilter;
        EnvelopeFollower compEnvelope;
        EnvelopeFollower duckEnvelope;
        FrequencyShifter freqShifter;

        // Pitch drift state
        float driftPhase = 0.0f;
        float driftValue = 0.0f;
        float driftTarget = 0.0f;

        // Freeze state
        bool frozen = false;
        float freezeCrossfade = 0.0f;

        void reset() {
            lpFilter.reset();
            hpFilter.reset();
            tiltFilter.reset();
            compEnvelope.reset();
            duckEnvelope.reset();
            freqShifter.reset();
            driftPhase = driftValue = driftTarget = 0.0f;
            frozen = false;
            freezeCrossfade = 0.0f;
        }
    };

    std::array<ChannelState, 2> channels;
    std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist{-1.0f, 1.0f};

    // Feedback filter parameters
    float lpFreq = 8000.0f;
    float hpFreq = 80.0f;
    float lpResonance = 0.0f;
    float hpResonance = 0.0f;

    // Feedback saturation/compression
    float driveAmount = 0.0f;
    float compAmount = 0.0f;

    // Tilt EQ (-1 = dark, +1 = bright)
    float tiltAmount = 0.0f;

    // Ducking
    float duckAmount = 0.0f;
    float duckRelease = 0.1f; // seconds

    // Pitch drift
    float driftAmount = 0.0f; // cents
    float driftRate = 0.5f;

    // Freeze
    bool freezeEnabled = false;

    // Frequency shift (Ghost mode)
    float freqShiftHz = 0.0f;
    bool freqShiftMode = false;

    float sampleRate = 44100.0f;

    void configure(float sr) {
        sampleRate = sr;
    }

    void reset() {
        for (auto& ch : channels) ch.reset();
    }

    /**
     * Process feedback signal through the feedback processing chain
     * Called on the signal AFTER it exits the delay line, BEFORE it's fed back
     *
     * @param input The delayed signal
     * @param channel 0=left, 1=right
     * @param inputLevel Current input level for ducking
     * @return Processed feedback signal
     */
    float process(float input, int channel, float inputLevel = 0.0f) {
        auto& ch = channels[channel];
        float out = input;

        // 1. Frequency shifting (Ghost mode) — applied instead of pitch shifting
        if (freqShiftMode && std::abs(freqShiftHz) > 0.01f) {
            out = ch.freqShifter.process(out, freqShiftHz, sampleRate);
        }

        // 2. Low-pass filter (prevent HF cascade buildup)
        if (lpFreq < 19000.0f) {
            out = ch.lpFilter.processMode(out, SVFilter::LOWPASS, lpFreq, lpResonance, sampleRate);
        }

        // 3. High-pass filter (prevent LF mud)
        if (hpFreq > 25.0f) {
            out = ch.hpFilter.processMode(out, SVFilter::HIGHPASS, hpFreq, hpResonance, sampleRate);
        }

        // 4. Tilt EQ (progressive darkening or brightening per repeat)
        if (std::abs(tiltAmount) > 0.01f) {
            float tiltFreq = 1000.0f;
            auto tiltOut = ch.tiltFilter.process(out, tiltFreq, 0.3f, sampleRate);
            // tiltAmount: -1 = dark (boost LP, cut HP), +1 = bright (cut LP, boost HP)
            out = tiltOut.lp * (1.0f - tiltAmount) + tiltOut.hp * (1.0f + tiltAmount);
        }

        // 5. Saturation (tanh soft clip with variable drive)
        if (driveAmount > 0.001f) {
            float drive = 1.0f + driveAmount * 4.0f;
            out = std::tanh(out * drive) / drive;
        }

        // 6. Compression (simple envelope-following limiter)
        if (compAmount > 0.001f) {
            float attackCoeff = 1.0f - std::exp(-1.0f / (sampleRate * 0.001f)); // 1ms attack
            float releaseCoeff = 1.0f - std::exp(-1.0f / (sampleRate * 0.05f)); // 50ms release
            float env = ch.compEnvelope.process(out, attackCoeff, releaseCoeff);
            float threshold = 1.0f - compAmount * 0.5f;
            if (env > threshold) {
                float gain = threshold / env;
                gain = gain + (1.0f - gain) * (1.0f - compAmount);
                out *= gain;
            }
        }

        // 7. Ducking (reduce feedback when input is present) — skip during freeze
        if (duckAmount > 0.001f && !freezeEnabled) {
            float duckAttack = 1.0f - std::exp(-1.0f / (sampleRate * 0.005f)); // 5ms attack
            float duckReleaseCoeff = 1.0f - std::exp(-1.0f / (sampleRate * duckRelease));
            float env = ch.duckEnvelope.process(inputLevel, duckAttack, duckReleaseCoeff);
            float duckGain = 1.0f - duckAmount * std::min(env * 5.0f, 1.0f);
            out *= std::max(duckGain, 0.0f);
        }

        // 8. Freeze handling
        if (freezeEnabled) {
            ch.freezeCrossfade = std::min(ch.freezeCrossfade + 1.0f / (sampleRate * 0.01f), 1.0f);
        } else {
            ch.freezeCrossfade = std::max(ch.freezeCrossfade - 1.0f / (sampleRate * 0.01f), 0.0f);
        }

        return out;
    }

    /**
     * Get per-iteration pitch drift in cents
     */
    float getPitchDrift(int channel) {
        if (driftAmount < 0.01f) return 0.0f;

        auto& ch = channels[channel];
        ch.driftPhase += driftRate / sampleRate;
        if (ch.driftPhase >= 1.0f) {
            ch.driftPhase -= 1.0f;
            ch.driftTarget = dist(rng) * driftAmount;
        }

        // Smooth interpolation to target
        ch.driftValue += (ch.driftTarget - ch.driftValue) * 0.001f;
        return ch.driftValue;
    }

    /**
     * Check if freeze is active (pitch should be disabled in feedback)
     */
    bool isFreezeActive() const {
        return freezeEnabled;
    }

    /**
     * Get freeze crossfade amount for smooth transition
     */
    float getFreezeCrossfade(int channel) const {
        return channels[channel].freezeCrossfade;
    }
};

} // namespace CurveAndDrag
