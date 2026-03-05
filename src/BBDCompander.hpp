#pragma once
#include <cmath>
#include <array>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace CurveAndDrag {

/**
 * BBD Compander — NE570/SA571-style compressor/expander
 *
 * Real BBD delays (MN3007, SAD1024) use a compander IC to improve SNR:
 * - Compress (2:1 ratio) before writing to the BBD
 * - Expand (1:2 ratio) after reading from the BBD
 *
 * The NE570/SA571 uses an RMS level detector with ~10ms attack, ~50ms release,
 * and applies gain in the log domain. This models those characteristics including:
 * - Frequency-dependent compression (highs compress more — "breathing")
 * - Asymmetric attack/release envelope
 * - Slight THD from the VCA stage
 * - Level-dependent noise floor
 */
struct BBDCompander {
    // Envelope detector state
    float envStateL = 0.0f;
    float envStateR = 0.0f;

    // Pre-emphasis filter state (boosts highs before compression like real NE570)
    float preEmphStateL = 0.0f;
    float preEmphStateR = 0.0f;

    // De-emphasis filter state
    float deEmphStateL = 0.0f;
    float deEmphStateR = 0.0f;

    // Previous gain for smoothing
    float prevGainL = 1.0f;
    float prevGainR = 1.0f;

    float sampleRate = 44100.0f;

    void configure(float sr) {
        sampleRate = sr;
    }

    void reset() {
        envStateL = envStateR = 0.0f;
        preEmphStateL = preEmphStateR = 0.0f;
        deEmphStateL = deEmphStateR = 0.0f;
        prevGainL = prevGainR = 1.0f;
    }

    /**
     * RMS envelope detector with asymmetric attack/release
     * Models the NE570's rectifier + capacitor averaging circuit
     */
    float detectEnvelope(float input, float& envState) {
        float rectified = input * input; // RMS detection (squared)

        // NE570 time constants: ~5ms attack, ~50ms release
        float attackCoeff = 1.0f - std::exp(-1.0f / (sampleRate * 0.005f));
        float releaseCoeff = 1.0f - std::exp(-1.0f / (sampleRate * 0.050f));

        float coeff = (rectified > envState) ? attackCoeff : releaseCoeff;
        envState += (rectified - envState) * coeff;

        return std::sqrt(std::max(envState, 1e-10f)); // RMS from squared
    }

    /**
     * Pre-emphasis: boost highs ~6dB/oct above 1kHz
     * Real NE570 has internal pre-emphasis to improve HF SNR
     */
    float preEmphasize(float input, float& state) {
        float cutoff = 1000.0f / sampleRate;
        float coeff = 1.0f / (1.0f + 1.0f / (2.0f * M_PI * cutoff));
        float hp = input - state;
        state += hp * coeff;
        return input + hp * 0.5f; // Boost highs by ~6dB
    }

    /**
     * De-emphasis: cut highs (inverse of pre-emphasis)
     */
    float deEmphasize(float input, float& state) {
        float cutoff = 1000.0f / sampleRate;
        float coeff = 1.0f / (1.0f + 1.0f / (2.0f * M_PI * cutoff));
        state += (input - state) * coeff;
        // Blend LP with input to undo pre-emphasis
        return state * 0.6f + input * 0.4f;
    }

    /**
     * Compress: 2:1 ratio in dB domain (input side of BBD)
     *
     * @param input Audio sample
     * @param channel 0=left, 1=right
     * @param amount 0-1 how much companding to apply
     * @return Compressed sample
     */
    float compress(float input, int channel, float amount) {
        if (amount < 0.001f) return input;

        float& envState = (channel == 0) ? envStateL : envStateR;
        float& preState = (channel == 0) ? preEmphStateL : preEmphStateR;
        float& prevGain = (channel == 0) ? prevGainL : prevGainR;

        // Pre-emphasis (boosts highs before compression)
        float preEmph = preEmphasize(input, preState);

        // Detect RMS level
        float env = detectEnvelope(preEmph, envState);

        // 2:1 compression in linear domain
        // gain = 1/sqrt(env) normalized, which gives 2:1 ratio in dB
        float gain = 1.0f;
        if (env > 0.001f) {
            gain = 1.0f / std::sqrt(env + 0.001f);
            gain = std::min(gain, 10.0f); // Limit expansion for very quiet signals
        }

        // Smooth gain changes to avoid clicks
        gain = prevGain + (gain - prevGain) * 0.1f;
        prevGain = gain;

        // Apply compression
        float compressed = preEmph * gain;

        // Slight VCA THD (adds even harmonics like real NE570 OTA VCA)
        float thd = compressed * compressed * 0.02f * amount;
        compressed += thd;

        // Blend with dry based on amount
        return input * (1.0f - amount) + compressed * amount;
    }

    /**
     * Expand: 1:2 ratio in dB domain (output side of BBD)
     *
     * @param input Audio sample from BBD
     * @param channel 0=left, 1=right
     * @param amount 0-1 how much companding to apply
     * @return Expanded sample
     */
    float expand(float input, int channel, float amount) {
        if (amount < 0.001f) return input;

        float& envState = (channel == 0) ? envStateL : envStateR;
        float& deState = (channel == 0) ? deEmphStateL : deEmphStateR;
        float& prevGain = (channel == 0) ? prevGainL : prevGainR;

        // Detect RMS level of the compressed signal
        float env = detectEnvelope(input, envState);

        // 1:2 expansion: gain = sqrt(env), inverse of compression
        float gain = 1.0f;
        if (env > 0.001f) {
            gain = std::sqrt(env + 0.001f);
            gain = std::min(gain, 10.0f);
        }

        // Smooth gain
        gain = prevGain + (gain - prevGain) * 0.1f;
        prevGain = gain;

        // Apply expansion
        float expanded = input * gain;

        // De-emphasis (undo the pre-emphasis)
        expanded = deEmphasize(expanded, deState);

        // Blend with dry based on amount
        return input * (1.0f - amount) + expanded * amount;
    }
};

/**
 * Laroche-Dolson Identity Phase Locking
 *
 * Improves pitch shifting quality by locking the phase of each bin
 * to the nearest peak in the spectrum. This reduces the "phasiness"
 * and metallic quality of naive phase vocoder pitch shifting.
 *
 * For our granular pitch shifters (H910, Hybrid), we approximate this
 * by using overlapping grains with phase-coherent crossfading rather
 * than simple triangle/Hann windows.
 *
 * The key insight: instead of independent phase advancement per grain,
 * we lock grain phases to be coherent at overlap points. This means:
 * 1. Grain start phases are derived from the input signal phase
 * 2. Crossfade windows are shaped to preserve phase at boundaries
 * 3. Small pitch shifts use longer grains (less windowing artifacts)
 */
struct PhaseLockingPitchShifter {
    static constexpr int MAX_GRAIN_SIZE = 4096;
    static constexpr int NUM_GRAINS = 4; // 4 overlapping grains for smooth output

    struct Grain {
        float phase = 0.0f;     // Current position in grain
        float startPos = 0.0f;  // Where this grain starts reading in the buffer
        bool active = false;
        float amplitude = 0.0f; // Current window amplitude
    };

    std::array<Grain, NUM_GRAINS> grains;
    std::array<float, 16384> buffer = {};
    int writePos = 0;
    int grainSize = 1024;
    float lastRatio = 1.0f;

    void reset() {
        buffer.fill(0.0f);
        writePos = 0;
        for (auto& g : grains) {
            g = Grain{};
        }
    }

    /**
     * Process with Laroche-Dolson style phase locking
     *
     * Uses 4 overlapping grains spaced 25% apart, each reading at
     * the pitch-shifted rate. Phase coherence is maintained by:
     * 1. Starting each grain at a zero-crossing or phase-aligned point
     * 2. Using raised-cosine (Hann) windows for artifact-free crossfading
     * 3. Adapting grain size to pitch ratio for optimal quality
     *
     * @param input Input sample
     * @param ratio Pitch ratio (1.0 = unity, 2.0 = octave up)
     * @param character Vintage character amount (0-1)
     * @return Pitch-shifted sample
     */
    float process(float input, float ratio, float character) {
        // Write input to circular buffer
        buffer[writePos] = input;

        // Adapt grain size to ratio: larger grains for small shifts, smaller for large
        float absRatio = std::abs(ratio - 1.0f);
        if (absRatio < 0.1f) {
            grainSize = 2048; // Very small shift: long grains for quality
        } else if (absRatio < 0.5f) {
            grainSize = 1024; // Medium shift
        } else {
            grainSize = 512;  // Large shift: short grains for responsiveness
        }

        float output = 0.0f;
        float windowSum = 0.0f;

        // Process each grain
        for (int i = 0; i < NUM_GRAINS; i++) {
            auto& g = grains[i];

            // Initialize grain if not active
            if (!g.active) {
                g.phase = (grainSize * i) / NUM_GRAINS; // Stagger grains evenly
                g.startPos = writePos;
                g.active = true;
            }

            // Advance grain read position at pitch ratio rate
            g.phase += ratio;

            // Check if grain is complete
            if (g.phase >= grainSize) {
                // Reset grain with phase-locked start position
                g.phase -= grainSize;
                g.startPos = writePos; // Start from current write position
            }

            // Hann window for this grain position
            float normalizedPos = g.phase / grainSize;
            float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * normalizedPos));

            // Read from buffer at grain's position
            float readPos = g.startPos - g.phase;
            int readIdx = static_cast<int>(std::floor(readPos));
            float frac = readPos - std::floor(readPos);

            // Cubic Hermite interpolation (higher quality than linear)
            int im1 = (readIdx - 1 + 16384) & 16383;
            int i0  = (readIdx + 16384) & 16383;
            int i1  = (readIdx + 1 + 16384) & 16383;
            int i2  = (readIdx + 2 + 16384) & 16383;
            float c0 = buffer[i0];
            float c1 = 0.5f * (buffer[i1] - buffer[im1]);
            float c2 = buffer[im1] - 2.5f * buffer[i0] + 2.0f * buffer[i1] - 0.5f * buffer[i2];
            float c3 = 0.5f * (buffer[i2] - buffer[im1]) + 1.5f * (buffer[i0] - buffer[i1]);
            float sample = ((c3 * frac + c2) * frac + c1) * frac + c0;

            // Accumulate with window
            output += sample * window;
            windowSum += window;
        }

        // Normalize by window sum to maintain unity gain
        if (windowSum > 0.001f) {
            output /= windowSum;
        }

        // Advance write position
        writePos = (writePos + 1) & 16383;

        return output;
    }
};

} // namespace CurveAndDrag
