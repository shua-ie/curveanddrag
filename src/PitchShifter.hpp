#pragma once
#include <rack.hpp>
#include <vector>
#include <complex>
#include <cmath>

namespace CurveAndDrag {

/**
 * PitchShifter - Phase vocoder based pitch shifting engine
 * 
 * Implements a real-time phase vocoder algorithm for high-quality
 * pitch shifting with minimal artifacts.
 */
class PitchShifter {
public:
    PitchShifter() {
        // Initialize with default values
        sampleRate = 44100.f;
        bufferSize = 2048;
        hopSize = 512;
        pitchShift = 1.0f;
        detuneDriftAmount = 0.0f;
        detuneDriftPhase = 0.0f;
        pitchMode = 0;              // Default to BBD mode
        characterAmount = 0.0f;     // No character processing by default

        // Allocate buffers
        reset();
    }

    /**
     * Reset all internal buffers and state
     */
    void reset() {
        // Allocate input and output buffers
        inputBuffer.resize(bufferSize * 2, 0.0f);
        outputBuffer.resize(bufferSize * 2, 0.0f);
        
        // Initialize phase data
        analysisPhase.resize(bufferSize / 2 + 1, 0.0f);
        synthesisPhase.resize(bufferSize / 2 + 1, 0.0f);
        
        // Reset counters
        inputPos = 0;
        outputPos = 0;
        frameCount = 0;
        
        // Allocate FFT data
        fftBufferR.resize(bufferSize);
        fftBufferI.resize(bufferSize);
        
        // Initialize windows
        analysisWindow.resize(bufferSize);
        synthesisWindow.resize(bufferSize);
        
        // Create Hann window
        for (int i = 0; i < bufferSize; i++) {
            float windowValue = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / bufferSize);
            analysisWindow[i] = windowValue;
            synthesisWindow[i] = windowValue;
        }
        
        // Normalize windows for perfect reconstruction
        float windowSum = 0.0f;
        for (int i = 0; i < bufferSize; i += hopSize) {
            windowSum += analysisWindow[i] * synthesisWindow[i];
        }
        
        float windowScaleFactor = 1.0f / windowSum * hopSize;
        for (int i = 0; i < bufferSize; i++) {
            analysisWindow[i] *= windowScaleFactor;
            synthesisWindow[i] *= windowScaleFactor;
        }
    }

    /**
     * Configure the pitch shifter
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
     * Set pitch shift amount in semitones
     * 
     * @param semitones Pitch shift in semitones (-12 to 12)
     */
    void setPitchShiftSemitones(float semitones) {
        // Convert semitones to ratio
        pitchShift = std::pow(2.0f, semitones / 12.0f);
    }

    /**
     * Set pitch shift amount in cents
     * 
     * @param cents Pitch shift in cents (-1200 to 1200)
     */
    void setPitchShiftCents(float cents) {
        // Convert cents to ratio
        pitchShift = std::pow(2.0f, cents / 1200.0f);
    }

    /**
     * Set the detune drift amount and update the LFO
     * 
     * @param amount Detune amount (0.0 to 1.0)
     * @param rate LFO rate in Hz (0.0 to 1.0)
     */
    void setDetuneDrift(float amount, float rate) {
        detuneDriftAmount = amount;
        detuneDriftRate = rate;
    }

    /**
     * Set pitch shifting mode (BBD, H910, Varispeed, Hybrid)
     * 
     * @param mode Pitch shifting mode
     */
    void setPitchMode(int mode) {
        pitchMode = clamp(mode, 0, 3);
    }

    /**
     * Set character/vintage modeling amount
     * 
     * @param character Character amount (0.0 to 1.0)
     */
    void setCharacter(float character) {
        characterAmount = clamp(character, 0.0f, 1.0f);
    }

    /**
     * Process a single audio sample through the pitch shifter
     * 
     * @param input Input audio sample
     * @param channel Channel index (0 = left, 1 = right)
     * @return Processed output sample
     */
    float process(float input, int channel) {
        // Update LFO for detuning
        updateDetuneDrift();
        
        // Apply character-based pre-processing
        float processedInput = applyCharacterProcessing(input);
        
        // Write input to buffer
        inputBuffer[inputPos] = processedInput;
        inputPos = (inputPos + 1) % inputBuffer.size();
        
        // Every hopSize samples, perform FFT processing
        if ((inputPos % hopSize) == 0) {
            processFrame();
            frameCount++;
        }
        
        // Read from output buffer
        float output = outputBuffer[outputPos];
        outputBuffer[outputPos] = 0.0f; // Clear the buffer
        outputPos = (outputPos + 1) % outputBuffer.size();
        
        // Apply character-based post-processing
        return applyCharacterPostProcessing(output);
    }

    /**
     * Process a single audio sample through the pitch shifter (legacy interface)
     * 
     * @param input Input audio sample
     * @return Processed output sample
     */
    float process(float input) {
        return process(input, 0);
    }

private:
    // Settings
    float sampleRate;
    int bufferSize;
    int hopSize;
    float pitchShift;
    
    // Detuning parameters
    float detuneDriftAmount;
    float detuneDriftRate;
    float detuneDriftPhase;
    
    // Pitch mode and character parameters
    int pitchMode;               // 0=BBD, 1=H910, 2=Varispeed, 3=Hybrid
    float characterAmount;       // Character/vintage modeling amount
    
    // Buffer positions
    int inputPos;
    int outputPos;
    int frameCount;
    
    // Audio buffers
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    
    // FFT data
    std::vector<float> fftBufferR;
    std::vector<float> fftBufferI;
    std::vector<float> analysisPhase;
    std::vector<float> synthesisPhase;
    
    // Windows
    std::vector<float> analysisWindow;
    std::vector<float> synthesisWindow;

    /**
     * Update the detune drift LFO
     */
    void updateDetuneDrift() {
        if (detuneDriftAmount > 0.0f && detuneDriftRate > 0.0f) {
            // Update LFO phase
            detuneDriftPhase += detuneDriftRate / sampleRate;
            if (detuneDriftPhase >= 1.0f) {
                detuneDriftPhase -= 1.0f;
            }
            
            // Calculate detune factor - subtle sine modulation
            float detuneModulation = std::sin(2.0f * M_PI * detuneDriftPhase);
            float detuneAmount = 1.0f + detuneModulation * detuneDriftAmount * 0.01f;
            
            // Apply to the pitch shift
            float basePitchShift = pitchShift;
            pitchShift = basePitchShift * detuneAmount;
        }
    }

    /**
     * Process a complete frame using FFT-based phase vocoder
     */
    void processFrame() {
        // Extract frame from input buffer with analysis window
        int frameStart = (inputPos - hopSize + inputBuffer.size()) % inputBuffer.size();
        for (int i = 0; i < bufferSize; i++) {
            int bufIdx = (frameStart + i) % inputBuffer.size();
            float windowedSample = inputBuffer[bufIdx] * analysisWindow[i];
            fftBufferR[i] = windowedSample;
            fftBufferI[i] = 0.0f;
        }
        
        // Perform forward FFT
        performFFT(fftBufferR, fftBufferI, bufferSize, false);
        
        // Convert to polar coordinates
        std::vector<float> magnitude(bufferSize / 2 + 1);
        std::vector<float> phase(bufferSize / 2 + 1);
        
        for (int i = 0; i <= bufferSize / 2; i++) {
            magnitude[i] = std::sqrt(fftBufferR[i] * fftBufferR[i] + fftBufferI[i] * fftBufferI[i]);
            phase[i] = std::atan2(fftBufferI[i], fftBufferR[i]);
        }
        
        // Calculate phase difference and instantaneous frequency
        std::vector<float> phaseDiff(bufferSize / 2 + 1);
        for (int i = 0; i <= bufferSize / 2; i++) {
            float expectedPhase = i * 2.0f * M_PI * hopSize / bufferSize;
            float phaseDelta = phase[i] - analysisPhase[i];
            
            // Unwrap phase difference
            phaseDiff[i] = phaseDelta - expectedPhase;
            phaseDiff[i] = phaseDiff[i] - std::round(phaseDiff[i] / (2.0f * M_PI)) * 2.0f * M_PI;
            phaseDiff[i] = phaseDiff[i] + expectedPhase;
            
            // Store current phase for next frame
            analysisPhase[i] = phase[i];
        }
        
        // Apply pitch shifting in frequency domain
        for (int i = 0; i <= bufferSize / 2; i++) {
            float newBinPos = i * pitchShift;
            int lowerBin = static_cast<int>(std::floor(newBinPos));
            int upperBin = lowerBin + 1;
            float fraction = newBinPos - lowerBin;
            
            if (lowerBin >= 0 && upperBin <= bufferSize / 2) {
                // Linear interpolation between bins
                float lowerWeight = 1.0f - fraction;
                float upperWeight = fraction;
                
                // Update magnitude using linear interpolation
                float newMag = magnitude[i];
                
                // Update phase
                float newPhaseDiff = phaseDiff[i];
                
                // Accumulate in output buffers
                if (lowerBin > 0 && lowerBin < bufferSize / 2) {
                    float phaseIncrement = newPhaseDiff * pitchShift;
                    synthesisPhase[lowerBin] += phaseIncrement;
                    
                    // Convert back to rectangular form
                    float mag = newMag * lowerWeight;
                    float phase = synthesisPhase[lowerBin];
                    fftBufferR[lowerBin] += mag * std::cos(phase);
                    fftBufferI[lowerBin] += mag * std::sin(phase);
                }
                
                if (upperBin > 0 && upperBin < bufferSize / 2) {
                    float phaseIncrement = newPhaseDiff * pitchShift;
                    synthesisPhase[upperBin] += phaseIncrement;
                    
                    // Convert back to rectangular form
                    float mag = newMag * upperWeight;
                    float phase = synthesisPhase[upperBin];
                    fftBufferR[upperBin] += mag * std::cos(phase);
                    fftBufferI[upperBin] += mag * std::sin(phase);
                }
            }
        }
        
        // Zero out the negative frequencies (conjugate symmetric for real signals)
        for (int i = bufferSize / 2 + 1; i < bufferSize; i++) {
            fftBufferR[i] = fftBufferR[bufferSize - i];
            fftBufferI[i] = -fftBufferI[bufferSize - i];
        }
        
        // Perform inverse FFT
        performFFT(fftBufferR, fftBufferI, bufferSize, true);
        
        // Apply synthesis window and overlap-add to output buffer
        // Note: frameEnd variable was removed as it was unused
        for (int i = 0; i < bufferSize; i++) {
            int bufIdx = (outputPos + i) % outputBuffer.size();
            float windowedSample = fftBufferR[i] * synthesisWindow[i];
            outputBuffer[bufIdx] += windowedSample;
        }
    }

    /**
     * Simple FFT implementation
     * 
     * @param real Real part of the signal
     * @param imag Imaginary part of the signal
     * @param n FFT size
     * @param inverse True for inverse FFT, false for forward FFT
     */
    void performFFT(std::vector<float>& real, std::vector<float>& imag, int n, bool inverse) {
        // Bit reversal permutation
        int bits = static_cast<int>(std::log2(n));
        for (int i = 0; i < n; i++) {
            int j = 0;
            for (int k = 0; k < bits; k++) {
                j = (j << 1) | ((i >> k) & 1);
            }
            if (j > i) {
                std::swap(real[i], real[j]);
                std::swap(imag[i], imag[j]);
            }
        }
        
        // Cooley-Tukey FFT algorithm
        for (int step = 2; step <= n; step *= 2) {
            float theta = 2.0f * M_PI / step * (inverse ? -1.0f : 1.0f);
            for (int m = 0; m < n; m += step) {
                for (int k = 0; k < step / 2; k++) {
                    float thetaK = theta * k;
                    float wR = std::cos(thetaK);
                    float wI = std::sin(thetaK);
                    
                    int a = m + k;
                    int b = m + k + step / 2;
                    
                    float aR = real[a];
                    float aI = imag[a];
                    float bR = real[b];
                    float bI = imag[b];
                    
                    float tR = wR * bR - wI * bI;
                    float tI = wR * bI + wI * bR;
                    
                    real[a] = aR + tR;
                    imag[a] = aI + tI;
                    real[b] = aR - tR;
                    imag[b] = aI - tI;
                }
            }
        }
        
        // Normalize if inverse FFT
        if (inverse) {
            for (int i = 0; i < n; i++) {
                real[i] /= n;
                imag[i] /= n;
            }
        }
    }

    /**
     * Apply character-based pre-processing based on pitch mode
     * 
     * @param input Input sample
     * @return Processed sample
     */
    float applyCharacterProcessing(float input) {
        if (characterAmount < 0.01f) {
            return input;
        }
        
        float processed = input;
        
        switch (pitchMode) {
            case 0: // BBD (Bucket Brigade Device)
                // Add subtle sample-and-hold artifacts and clock noise
                processed += (std::sin(frameCount * 0.1f) * 0.001f) * characterAmount;
                processed = processed * (1.0f - characterAmount * 0.1f) + 
                           std::tanh(processed * 1.5f) * characterAmount * 0.1f;
                break;
                
            case 1: // H910 Harmonizer
                // Add subtle digital quantization and aliasing
                if (characterAmount > 0.5f) {
                    float bits = 16.0f - characterAmount * 4.0f; // Reduce bit depth
                    float scale = std::pow(2.0f, bits - 1.0f);
                    processed = std::round(processed * scale) / scale;
                }
                break;
                
            case 2: // Varispeed
                // Add tape-like saturation and wow
                processed = std::tanh(processed * (1.0f + characterAmount)) / (1.0f + characterAmount);
                processed += std::sin(frameCount * 0.01f) * characterAmount * 0.005f;
                break;
                
            case 3: // Hybrid
                // Combine elements from all modes
                processed = std::tanh(processed * (1.0f + characterAmount * 0.3f)) / (1.0f + characterAmount * 0.3f);
                processed += (std::sin(frameCount * 0.05f) * 0.002f) * characterAmount;
                break;
        }
        
        return processed;
    }

    /**
     * Apply character-based post-processing
     * 
     * @param output Output sample
     * @return Processed sample
     */
    float applyCharacterPostProcessing(float output) {
        if (characterAmount < 0.01f) {
            return output;
        }
        
        float processed = output;
        
        switch (pitchMode) {
            case 0: { // BBD
                // Add subtle low-pass filtering
                static float bbdLowpass = 0.0f;
                float cutoff = 1.0f - characterAmount * 0.3f;
                bbdLowpass += (processed - bbdLowpass) * cutoff;
                processed = bbdLowpass;
                break;
            }
                
            case 1: { // H910
                // Add slight high-frequency emphasis
                static float h910Highpass = 0.0f;
                h910Highpass += (processed - h910Highpass) * 0.95f;
                processed += (processed - h910Highpass) * characterAmount * 0.1f;
                break;
            }
                
            case 2: { // Varispeed
                // Add subtle filtering and warmth
                static float varisLowpass = 0.0f;
                varisLowpass += (processed - varisLowpass) * 0.8f;
                processed = processed * (1.0f - characterAmount * 0.3f) + varisLowpass * characterAmount * 0.3f;
                break;
            }
                
            case 3: { // Hybrid
                // Gentle overall processing
                processed = std::tanh(processed * 0.9f) * 1.1f;
                break;
            }
        }
        
        return processed;
    }
};

} // namespace CurveAndDrag
