#pragma once
#include <cmath>
#include <array>
#include <complex>
#include "math_constants.h"

namespace CurveAndDrag {

/**
 * Spectral Pitch Shifter — FFT-based phase vocoder with formant preservation
 *
 * Uses WOLA (Weighted Overlap-Add) phase vocoder with:
 * - Cooley-Tukey radix-2 FFT (no external dependencies)
 * - Peak-picking pitch shift (Laroche-Dolson 1999)
 * - Formant preservation via real cepstrum (liftered spectral envelope)
 * - Independent time stretching via synthesis hop manipulation
 *
 * Quality-first design: 4096-point FFT, 75% overlap, Hann windowing.
 *
 * Latency: FFT_SIZE samples (~93ms at 44.1kHz).
 */
class SpectralPitchShifter {
public:
    static constexpr int FFT_SIZE = 4096;
    static constexpr int HALF_FFT = FFT_SIZE / 2 + 1;
    static constexpr int HOP_SIZE = FFT_SIZE / 4; // 75% overlap
    static constexpr int LIFTER_CUTOFF = 30; // Cepstral lifter for formant envelope

    SpectralPitchShifter() {
        precomputeTwiddles();
        computeWindow();
        reset();
    }

    void reset() {
        inputBuffer.fill(0.0f);
        outputBuffer.fill(0.0f);
        lastPhase.fill(0.0f);
        sumPhase.fill(0.0f);
        inputPos = 0;
        hopCounter = 0;
        // Output write starts at the latency offset; read starts at 0.
        // This gives FFT_SIZE samples of latency for the first frame to be ready.
        outputWritePos = FFT_SIZE;
        outputReadPos = 0;
        // Track accumulated fractional output position for time stretch
        outputWritePosF = static_cast<float>(FFT_SIZE);
    }

    /**
     * Process a single sample with pitch shifting only.
     * Time stretching ratio is 1.0 (no stretch).
     */
    void process(float input, float ratio, float character,
                 float& output, float sampleRate) {
        processTimeStretch(input, ratio, 1.0f, character, output, sampleRate);
    }

    /**
     * Process a single sample with independent pitch shift and time stretch.
     *
     * @param input        Input sample
     * @param pitchRatio   Pitch ratio (2.0 = octave up, 0.5 = octave down)
     * @param stretchRatio Time stretch (2.0 = half speed, 0.5 = double speed)
     * @param character    Formant preservation amount (0 = robotic, 1 = full preservation)
     * @param output       Output sample (written by reference)
     * @param sampleRate   Current sample rate
     */
    void processTimeStretch(float input, float pitchRatio, float stretchRatio,
                           float character, float& output, float sampleRate) {
        // Write input to circular analysis buffer
        inputBuffer[inputPos] = input;
        inputPos = (inputPos + 1) & (INPUT_BUF_SIZE - 1);

        hopCounter++;

        // Process a new FFT frame every HOP_SIZE samples
        if (hopCounter >= HOP_SIZE) {
            hopCounter = 0;
            processFrame(pitchRatio, stretchRatio, character, sampleRate);
        }

        // Read from output buffer
        output = outputBuffer[outputReadPos];
        outputBuffer[outputReadPos] = 0.0f; // Clear after reading
        outputReadPos = (outputReadPos + 1) & (OUTPUT_BUF_SIZE - 1);
    }

private:
    static constexpr int INPUT_BUF_SIZE = FFT_SIZE * 2; // Must be power of 2
    static constexpr int OUTPUT_BUF_SIZE = FFT_SIZE * 4; // Extra room for time stretch

    // Circular buffers
    std::array<float, INPUT_BUF_SIZE> inputBuffer;
    std::array<float, OUTPUT_BUF_SIZE> outputBuffer;

    // Phase vocoder state
    std::array<float, HALF_FFT> lastPhase;
    std::array<float, HALF_FFT> sumPhase;

    // Buffer positions
    int inputPos = 0;
    int outputWritePos = 0;
    float outputWritePosF = 0.0f; // Fractional accumulator for time stretch
    int hopCounter = 0;
    int outputReadPos = 0;

    // Precomputed
    std::array<float, FFT_SIZE> window;
    std::array<std::complex<float>, FFT_SIZE / 2> twiddles;

    // Working buffers (avoid allocation in process)
    std::array<std::complex<float>, FFT_SIZE> fftWork;
    std::array<float, HALF_FFT> analysisMag;
    std::array<float, HALF_FFT> analysisFreq;
    std::array<float, HALF_FFT> synthMag;
    std::array<float, HALF_FFT> synthFreq;

    // Formant preservation working buffers (member to avoid stack overflow)
    std::array<std::complex<float>, FFT_SIZE> cepData;
    std::array<float, HALF_FFT> origEnvelope;
    std::array<float, HALF_FFT> shiftedEnvelope;

    // ===== FFT Engine (Cooley-Tukey radix-2) =====

    void precomputeTwiddles() {
        for (int i = 0; i < FFT_SIZE / 2; i++) {
            float angle = -2.0f * static_cast<float>(M_PI) * i / FFT_SIZE;
            twiddles[i] = std::complex<float>(std::cos(angle), std::sin(angle));
        }
    }

    void computeWindow() {
        for (int i = 0; i < FFT_SIZE; i++) {
            window[i] = 0.5f * (1.0f - std::cos(2.0f * static_cast<float>(M_PI) * i / FFT_SIZE));
        }
    }

    static int bitReverse(int x, int log2n) {
        int result = 0;
        for (int i = 0; i < log2n; i++) {
            result = (result << 1) | (x & 1);
            x >>= 1;
        }
        return result;
    }

    void fft(std::array<std::complex<float>, FFT_SIZE>& data, bool inverse) {
        constexpr int log2n = 12; // log2(4096)

        // Bit-reversal permutation
        for (int i = 0; i < FFT_SIZE; i++) {
            int j = bitReverse(i, log2n);
            if (i < j) std::swap(data[i], data[j]);
        }

        // Butterfly stages
        for (int stage = 1; stage <= log2n; stage++) {
            int halfSize = 1 << (stage - 1);
            int fullSize = 1 << stage;
            int twiddleStep = FFT_SIZE >> stage;

            for (int group = 0; group < FFT_SIZE; group += fullSize) {
                for (int k = 0; k < halfSize; k++) {
                    int twIdx = k * twiddleStep;
                    std::complex<float> tw = inverse ?
                        std::conj(twiddles[twIdx]) : twiddles[twIdx];

                    std::complex<float> even = data[group + k];
                    std::complex<float> odd = data[group + k + halfSize] * tw;

                    data[group + k] = even + odd;
                    data[group + k + halfSize] = even - odd;
                }
            }
        }

        if (inverse) {
            float scale = 1.0f / FFT_SIZE;
            for (int i = 0; i < FFT_SIZE; i++) {
                data[i] *= scale;
            }
        }
    }

    // ===== Spectral Envelope via Real Cepstrum =====

    void computeSpectralEnvelope(const std::array<float, HALF_FFT>& magnitude,
                                  std::array<float, HALF_FFT>& envelope) {
        // Log magnitude spectrum
        for (int i = 0; i < HALF_FFT; i++) {
            float logMag = std::log(std::max(magnitude[i], 1e-10f));
            cepData[i] = std::complex<float>(logMag, 0.0f);
        }
        // Mirror for full spectrum (conjugate symmetry of real signal)
        for (int i = HALF_FFT; i < FFT_SIZE; i++) {
            cepData[i] = cepData[FFT_SIZE - i];
        }

        // IFFT to get real cepstrum
        fft(cepData, true);

        // Lifter: zero out pitch-related coefficients, keep formant shape
        for (int i = LIFTER_CUTOFF; i < FFT_SIZE - LIFTER_CUTOFF + 1; i++) {
            cepData[i] = std::complex<float>(0.0f, 0.0f);
        }

        // FFT back to get smoothed spectral envelope
        fft(cepData, false);

        for (int i = 0; i < HALF_FFT; i++) {
            envelope[i] = std::exp(cepData[i].real());
        }
    }

    // ===== Phase Vocoder Frame Processing =====

    void processFrame(float pitchRatio, float stretchRatio, float character, float sampleRate) {
        float freqPerBin = sampleRate / FFT_SIZE;
        float expectedPhaseAdv = 2.0f * static_cast<float>(M_PI) * HOP_SIZE / FFT_SIZE;

        // === ANALYSIS: Window and FFT ===
        for (int i = 0; i < FFT_SIZE; i++) {
            int bufIdx = (inputPos - FFT_SIZE + i + INPUT_BUF_SIZE) & (INPUT_BUF_SIZE - 1);
            fftWork[i] = std::complex<float>(inputBuffer[bufIdx] * window[i], 0.0f);
        }

        fft(fftWork, false);

        // === Extract magnitude and true frequency for each bin ===
        for (int k = 0; k < HALF_FFT; k++) {
            float mag = std::abs(fftWork[k]);
            float phase = std::arg(fftWork[k]);

            // Phase difference from last frame
            float phaseDiff = phase - lastPhase[k];
            lastPhase[k] = phase;

            // Remove expected phase advance
            phaseDiff -= k * expectedPhaseAdv;

            // Wrap to [-pi, pi]
            phaseDiff = phaseDiff - 2.0f * static_cast<float>(M_PI) *
                std::round(phaseDiff / (2.0f * static_cast<float>(M_PI)));

            // True frequency = bin frequency + deviation
            float trueFreq = k * freqPerBin + phaseDiff * freqPerBin / expectedPhaseAdv;

            analysisMag[k] = mag;
            analysisFreq[k] = trueFreq;
        }

        // === PEAK-PICKING PITCH SHIFT (Laroche-Dolson) ===
        synthMag.fill(0.0f);
        synthFreq.fill(0.0f);

        // Identify peaks and shift them
        for (int k = 0; k < HALF_FFT; k++) {
            // Simple peak detection: bin k is a peak if magnitude > neighbors
            bool isPeak = (k == 0) ||
                          (k == HALF_FFT - 1) ||
                          (analysisMag[k] > analysisMag[k - 1] && analysisMag[k] >= analysisMag[k + 1]);

            if (!isPeak && analysisMag[k] < 0.001f) continue;

            // Target bin after pitch shift
            int targetBin = static_cast<int>(std::round(k * pitchRatio));
            if (targetBin < 0 || targetBin >= HALF_FFT) continue;

            // Find the region of influence for this peak
            int regionStart = k;
            int regionEnd = k;

            if (isPeak) {
                // Extend region backwards until we hit a valley or another peak
                for (int j = k - 1; j >= 0; j--) {
                    if (analysisMag[j] > analysisMag[j + 1]) break;
                    regionStart = j;
                }
                // Extend region forwards
                for (int j = k + 1; j < HALF_FFT; j++) {
                    if (analysisMag[j] > analysisMag[j - 1]) break;
                    regionEnd = j;
                }
            }

            // Shift the entire region
            for (int j = regionStart; j <= regionEnd; j++) {
                int shiftedBin = static_cast<int>(std::round(j * pitchRatio));
                if (shiftedBin >= 0 && shiftedBin < HALF_FFT) {
                    if (analysisMag[j] > synthMag[shiftedBin]) {
                        synthMag[shiftedBin] = analysisMag[j];
                        synthFreq[shiftedBin] = analysisFreq[j] * pitchRatio;
                    }
                }
            }
        }

        // === FORMANT PRESERVATION ===
        if (character > 0.01f) {
            computeSpectralEnvelope(analysisMag, origEnvelope);
            computeSpectralEnvelope(synthMag, shiftedEnvelope);

            for (int k = 0; k < HALF_FFT; k++) {
                if (shiftedEnvelope[k] > 1e-8f && synthMag[k] > 1e-10f) {
                    float correction = origEnvelope[k] / shiftedEnvelope[k];
                    correction = 1.0f + (correction - 1.0f) * character;
                    synthMag[k] *= correction;
                }
            }
        }

        // === SYNTHESIS: Phase accumulation and IFFT ===
        // Synthesis hop controls time stretch: synthHop > HOP_SIZE = slower, < = faster
        float synthHopF = HOP_SIZE * stretchRatio;
        int synthHop = std::max(static_cast<int>(std::round(synthHopF)), 1);
        float synthExpectedPhaseAdv = 2.0f * static_cast<float>(M_PI) * synthHop / FFT_SIZE;

        for (int k = 0; k < HALF_FFT; k++) {
            float phaseDev = (synthFreq[k] - k * freqPerBin) * synthExpectedPhaseAdv / freqPerBin;
            float phaseInc = k * synthExpectedPhaseAdv + phaseDev;
            sumPhase[k] += phaseInc;
            fftWork[k] = std::polar(synthMag[k], sumPhase[k]);
        }

        // Force DC and Nyquist bins to be real (conjugate symmetry requirement)
        fftWork[0] = std::complex<float>(fftWork[0].real(), 0.0f);
        fftWork[HALF_FFT - 1] = std::complex<float>(fftWork[HALF_FFT - 1].real(), 0.0f);

        // Mirror for full spectrum (conjugate symmetry)
        for (int k = HALF_FFT; k < FFT_SIZE; k++) {
            fftWork[k] = std::conj(fftWork[FFT_SIZE - k]);
        }

        // IFFT
        fft(fftWork, true);

        // === Overlap-add to output buffer ===
        // Normalization: scale by (synthHop / HOP_SIZE) to compensate for stretch
        float normFactor = (2.0f / 3.0f) * (static_cast<float>(HOP_SIZE) / synthHop);

        for (int i = 0; i < FFT_SIZE; i++) {
            int outIdx = (outputWritePos + i) & (OUTPUT_BUF_SIZE - 1);
            outputBuffer[outIdx] += fftWork[i].real() * window[i] * normFactor;
        }

        // Advance output write position using fractional accumulator
        // This keeps write and read in sync regardless of rounding
        outputWritePosF += synthHopF;
        // Normalize to prevent float precision loss after long runs (~24hrs)
        if (outputWritePosF >= static_cast<float>(OUTPUT_BUF_SIZE)) {
            outputWritePosF -= static_cast<float>(OUTPUT_BUF_SIZE);
        }
        outputWritePos = static_cast<int>(outputWritePosF) & (OUTPUT_BUF_SIZE - 1);
    }
};

} // namespace CurveAndDrag
