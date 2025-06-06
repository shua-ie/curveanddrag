#pragma once
#include <rack.hpp>
#include "plugin.hpp"
#include "DelayLine.hpp"
#include "PitchShifter.hpp"
#include "ScalaReader.hpp"
#include "TapeDelayProcessor.hpp"
#include "MTS_ESP.hpp"

using namespace rack;

namespace CurveAndDrag {

/**
 * @brief Parameter IDs for the CurveAndDrag module
 */
enum ParamIds {
    // Core delay parameters
    TIME_L_PARAM,
    TIME_R_PARAM,
    FEEDBACK_L_PARAM,
    FEEDBACK_R_PARAM,
    MIX_L_PARAM,
    MIX_R_PARAM,
    
    // Pitch shifting parameters
    PITCH_PARAM,
    DETUNE_DRIFT_PARAM,
    QUANTIZE_PARAM,
    MORPH_PARAM,
    
    // Sync and routing parameters
    SYNC_L_PARAM,
    SYNC_R_PARAM,
    CROSS_FEEDBACK_PARAM,
    SCALE_SELECT_PARAM,
    
    // Tape delay parameters
    TAPE_MODE_PARAM,
    WOW_RATE_PARAM,
    WOW_DEPTH_PARAM,
    FLUTTER_RATE_PARAM,
    FLUTTER_DEPTH_PARAM,
    WOW_WAVEFORM_PARAM,
    FLUTTER_WAVEFORM_PARAM,
    SATURATION_PARAM,
    HEAD_BUMP_FREQ_PARAM,
    HEAD_BUMP_GAIN_PARAM,
    ROLLOFF_FREQ_PARAM,
    ROLLOFF_RESONANCE_PARAM,
    TAPE_NOISE_PARAM,
    NOISE_AMOUNT_PARAM,
    
    // MTS-ESP microtuning parameters
    MTS_ENABLE_PARAM,
    
    NUM_PARAMS
};

/**
 * @brief Input IDs for the CurveAndDrag module
 */
enum InputIds {
    // Audio inputs
    LEFT_INPUT,
    RIGHT_INPUT,
    
    // CV inputs for core parameters
    TIME_L_CV_INPUT,
    TIME_R_CV_INPUT,
    FEEDBACK_L_CV_INPUT,
    FEEDBACK_R_CV_INPUT,
    MIX_L_CV_INPUT,
    MIX_R_CV_INPUT,
    PITCH_CV_INPUT,
    DETUNE_DRIFT_CV_INPUT,
    MORPH_CV_INPUT,
    
    // Trigger inputs
    TAP_L_TRIGGER_INPUT,
    TAP_R_TRIGGER_INPUT,
    
    // Tape delay CV inputs
    WOW_RATE_CV_INPUT,
    WOW_DEPTH_CV_INPUT,
    FLUTTER_RATE_CV_INPUT,
    SATURATION_CV_INPUT,
    NOISE_AMOUNT_CV_INPUT,
    
    NUM_INPUTS
};

/**
 * @brief Output IDs for the CurveAndDrag module
 */
enum OutputIds {
    LEFT_OUTPUT,
    RIGHT_OUTPUT,
    NUM_OUTPUTS
};

/**
 * @brief Light IDs for the CurveAndDrag module
 */
enum LightIds {
    // Status lights
    SYNC_L_LIGHT,
    SYNC_R_LIGHT,
    CROSS_FEEDBACK_LIGHT,
    QUANTIZE_LIGHT,
    TAPE_MODE_LIGHT,
    MTS_ACTIVE_LIGHT,
    
    // Level meter lights
    LEVEL_LIGHTS_L_START,
    LEVEL_LIGHTS_L_END = LEVEL_LIGHTS_L_START + 5,
    LEVEL_LIGHTS_R_START,
    LEVEL_LIGHTS_R_END = LEVEL_LIGHTS_R_START + 5,
    
    NUM_LIGHTS
};

/**
 * @brief Main CurveAndDrag module class
 * 
 * A stereo delay and pitch shifter with microtonal support via MTS-ESP.
 * Features tape-style delay effects, cross-feedback, and comprehensive
 * CV control over all parameters.
 */
class CurveAndDragModule : public Module {
public:
    /**
     * @brief Constructor - initializes the module
     */
    CurveAndDragModule();
    
    /**
     * @brief Destructor - ensures proper cleanup
     */
    ~CurveAndDragModule() = default;

    /**
     * @brief Reset module to initial state
     */
    void onReset() override;

    /**
     * @brief Handle sample rate changes
     */
    void onSampleRateChange() override;

    /**
     * @brief Main audio processing function
     * @param args Processing arguments from VCV Rack
     */
    void process(const ProcessArgs& args) override;

    /**
     * @brief Load module state from JSON
     * @param rootJ JSON object containing module state
     */
    void dataFromJson(json_t* rootJ) override;

    /**
     * @brief Save module state to JSON
     * @return JSON object containing module state
     */
    json_t* dataToJson() override;

private:
    // Audio processing components
    DelayLine leftDelay;
    DelayLine rightDelay;
    PitchShifter pitchShifter;
    ScalaReader scalaReader;
    TapeDelayProcessor tapeProcessor;
    MTSESPClient mtsClient;

    // Control processing
    dsp::SchmittTrigger leftTapTrigger;
    dsp::SchmittTrigger rightTapTrigger;
    dsp::Timer leftTapTimer;
    dsp::Timer rightTapTimer;

    // Level metering
    float leftLevelSmooth = 0.0f;
    float rightLevelSmooth = 0.0f;
    static constexpr float LEVEL_SMOOTH_RATE = 0.01f;

    // Tuning state
    std::string tuningSource = "12-TET";
    std::string tuningInfo = "Equal Temperament";
    float lastRawPitch = 0.0f;
    float lastQuantizedPitch = 0.0f;

    // Rate limiting for expensive operations
    int processCounter = 0;
    static constexpr int MTS_POLL_RATE = 1024; // Poll MTS every N samples
    static constexpr int LEVEL_UPDATE_RATE = 64; // Update levels every N samples

    /**
     * @brief Process delay-related parameters and CV inputs
     * @param sampleRate Current sample rate
     */
    void processDelayParameters(float sampleRate);

    /**
     * @brief Process tap tempo inputs
     * @param sampleRate Current sample rate
     */
    void processTapTempo(float sampleRate);

    /**
     * @brief Process pitch shifting parameters
     */
    void processPitchParameters();

    /**
     * @brief Process tape delay parameters
     */
    void processTapeParameters();

    /**
     * @brief Process scale selection and MTS-ESP integration
     */
    void processScaleSelection();

    /**
     * @brief Update level meters
     * @param leftInput Left channel input level
     * @param rightInput Right channel input level
     */
    void updateLevelMeters(float leftInput, float rightInput);

    /**
     * @brief Update status lights
     */
    void updateStatusLights();

    /**
     * @brief Clamp parameter value with CV input
     * @param paramId Parameter ID
     * @param cvInputId CV input ID
     * @param minVal Minimum value
     * @param maxVal Maximum value
     * @param defaultVal Default value if no CV
     * @return Clamped parameter value
     */
    float getClampedParam(int paramId, int cvInputId, float minVal, float maxVal, float defaultVal = 0.0f);

    /**
     * @brief Apply soft clipping to prevent harsh distortion
     * @param input Input sample
     * @return Soft-clipped output sample
     */
    inline float softClip(float input) {
        return std::tanh(input * 0.7f) / 0.7f;
    }

    /**
     * @brief Convert milliseconds to samples
     * @param ms Time in milliseconds
     * @param sampleRate Sample rate in Hz
     * @return Time in samples
     */
    inline float msToSamples(float ms, float sampleRate) {
        return (ms / 1000.0f) * sampleRate;
    }

    /**
     * @brief Convert samples to milliseconds
     * @param samples Time in samples
     * @param sampleRate Sample rate in Hz
     * @return Time in milliseconds
     */
    inline float samplesToMs(float samples, float sampleRate) {
        return (samples / sampleRate) * 1000.0f;
    }
};

// Constructor implementation
inline CurveAndDragModule::CurveAndDragModule() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    
    // Configure delay time parameters (1-2000ms)
    configParam(TIME_L_PARAM, 0.0f, 1.0f, 0.5f, "Left Delay Time", " ms", 0.0f, 2000.0f);
    configParam(TIME_R_PARAM, 0.0f, 1.0f, 0.5f, "Right Delay Time", " ms", 0.0f, 2000.0f);
    
    // Configure feedback parameters (0-110%)
    configParam(FEEDBACK_L_PARAM, 0.0f, 1.1f, 0.5f, "Left Feedback", "%", 0.0f, 100.0f);
    configParam(FEEDBACK_R_PARAM, 0.0f, 1.1f, 0.5f, "Right Feedback", "%", 0.0f, 100.0f);
    
    // Configure mix parameters (0-100%)
    configParam(MIX_L_PARAM, 0.0f, 1.0f, 0.5f, "Left Dry/Wet Mix", "%", 0.0f, 100.0f);
    configParam(MIX_R_PARAM, 0.0f, 1.0f, 0.5f, "Right Dry/Wet Mix", "%", 0.0f, 100.0f);
    
    // Configure pitch shift parameter (-1200 to 1200 cents)
    configParam(PITCH_PARAM, -1.0f, 1.0f, 0.0f, "Pitch Shift", " cents", 0.0f, 1200.0f, 0.0f);
    
    // Configure detune drift parameter (0-100%)
    configParam(DETUNE_DRIFT_PARAM, 0.0f, 1.0f, 0.0f, "Detune Drift", "%", 0.0f, 100.0f);
    
    // Configure quantize and morph parameters
    configParam(QUANTIZE_PARAM, 0.0f, 1.0f, 0.0f, "Quantize Pitch");
    configParam(MORPH_PARAM, 0.0f, 1.0f, 0.0f, "Raw/Quantized Morph", "%", 0.0f, 100.0f);
    
    // Configure sync and routing parameters
    configParam(SYNC_L_PARAM, 0.0f, 1.0f, 0.0f, "Left Sync");
    configParam(SYNC_R_PARAM, 0.0f, 1.0f, 0.0f, "Right Sync");
    configParam(CROSS_FEEDBACK_PARAM, 0.0f, 1.0f, 0.0f, "Cross-Feedback");
    configParam(SCALE_SELECT_PARAM, 0.0f, 2.0f, 0.0f, "Scale Select");
    
    // Configure tape delay parameters
    configParam(TAPE_MODE_PARAM, 0.0f, 1.0f, 0.0f, "Tape Mode");
    configParam(WOW_RATE_PARAM, 0.1f, 5.0f, 0.3f, "Wow Rate", " Hz");
    configParam(WOW_DEPTH_PARAM, 0.0f, 1.0f, 0.2f, "Wow Depth", "%", 0.0f, 100.0f);
    configParam(FLUTTER_RATE_PARAM, 1.0f, 5.0f, 2.7f, "Flutter Rate", " Hz");
    configParam(FLUTTER_DEPTH_PARAM, 0.0f, 1.0f, 0.1f, "Flutter Depth", "%", 0.0f, 100.0f);
    configParam(WOW_WAVEFORM_PARAM, 0.0f, 2.0f, 0.0f, "Wow Waveform");
    configParam(FLUTTER_WAVEFORM_PARAM, 0.0f, 2.0f, 0.0f, "Flutter Waveform");
    configParam(SATURATION_PARAM, 0.0f, 1.0f, 0.5f, "Tape Saturation", "%", 0.0f, 100.0f);
    configParam(HEAD_BUMP_FREQ_PARAM, 60.0f, 120.0f, 90.0f, "Head Bump Frequency", " Hz");
    configParam(HEAD_BUMP_GAIN_PARAM, 0.0f, 3.0f, 1.5f, "Head Bump Gain");
    configParam(ROLLOFF_FREQ_PARAM, 3000.0f, 12000.0f, 8000.0f, "High Frequency Rolloff", " Hz");
    configParam(ROLLOFF_RESONANCE_PARAM, 0.5f, 1.5f, 0.7f, "Rolloff Resonance");
    configParam(TAPE_NOISE_PARAM, 0.0f, 1.0f, 0.0f, "Tape Noise");
    configParam(NOISE_AMOUNT_PARAM, 0.0f, 0.1f, 0.01f, "Noise Amount", "%", 0.0f, 100.0f);
    
    // Configure MTS-ESP support
    configParam(MTS_ENABLE_PARAM, 0.0f, 1.0f, 0.0f, "MTS-ESP Enable");

    // Configure inputs
    configInput(LEFT_INPUT, "Left Audio");
    configInput(RIGHT_INPUT, "Right Audio");
    configInput(TIME_L_CV_INPUT, "Left Time CV");
    configInput(TIME_R_CV_INPUT, "Right Time CV");
    configInput(FEEDBACK_L_CV_INPUT, "Left Feedback CV");
    configInput(FEEDBACK_R_CV_INPUT, "Right Feedback CV");
    configInput(MIX_L_CV_INPUT, "Left Mix CV");
    configInput(MIX_R_CV_INPUT, "Right Mix CV");
    configInput(PITCH_CV_INPUT, "Pitch CV");
    configInput(DETUNE_DRIFT_CV_INPUT, "Detune Drift CV");
    configInput(MORPH_CV_INPUT, "Morph CV");
    configInput(TAP_L_TRIGGER_INPUT, "Left Tap Tempo");
    configInput(TAP_R_TRIGGER_INPUT, "Right Tap Tempo");
    configInput(WOW_RATE_CV_INPUT, "Wow Rate CV");
    configInput(WOW_DEPTH_CV_INPUT, "Wow Depth CV");
    configInput(FLUTTER_RATE_CV_INPUT, "Flutter Rate CV");
    configInput(SATURATION_CV_INPUT, "Saturation CV");
    configInput(NOISE_AMOUNT_CV_INPUT, "Noise Amount CV");
    
    // Configure outputs
    configOutput(LEFT_OUTPUT, "Left Audio");
    configOutput(RIGHT_OUTPUT, "Right Audio");
    
    // Initialize audio processing components
    float sampleRate = APP->engine->getSampleRate();
    leftDelay.configure(sampleRate);
    rightDelay.configure(sampleRate);
    pitchShifter.configure(sampleRate);
    tapeProcessor.configure(sampleRate);
    
    // Initialize scala reader with default 12-TET
    scalaReader.setDefaultScale();
    
    // Initialize triggers
    leftTapTrigger.reset();
    rightTapTrigger.reset();
}

// Method implementations
inline void CurveAndDragModule::onReset() {
    // Reset all audio processing components
    leftDelay.reset();
    rightDelay.reset();
    pitchShifter.reset();
    tapeProcessor.reset();
    
    // Reset timers and triggers
    leftTapTimer.reset();
    rightTapTimer.reset();
    leftTapTrigger.reset();
    rightTapTrigger.reset();
    
    // Reset level meters
    leftLevelSmooth = 0.0f;
    rightLevelSmooth = 0.0f;
    
    // Reset tuning state
    lastRawPitch = 0.0f;
    lastQuantizedPitch = 0.0f;
    
    // Reset process counter
    processCounter = 0;
}

inline void CurveAndDragModule::onSampleRateChange() {
    float sampleRate = APP->engine->getSampleRate();
    
    // Reconfigure all components for new sample rate
    leftDelay.configure(sampleRate);
    rightDelay.configure(sampleRate);
    pitchShifter.configure(sampleRate);
    tapeProcessor.configure(sampleRate);
}

inline void CurveAndDragModule::process(const ProcessArgs& args) {
    // Increment process counter for rate limiting
    processCounter++;
    
    // Poll MTS-ESP connection periodically
    if (processCounter % MTS_POLL_RATE == 0) {
        mtsClient.pollForMtsConnection();
    }
    
    // Process parameters
    processDelayParameters(args.sampleRate);
    processTapTempo(args.sampleRate);
    processPitchParameters();
    processTapeParameters();
    processScaleSelection();
    
    // Get input signals
    float leftInput = inputs[LEFT_INPUT].getVoltage();
    float rightInput = inputs[RIGHT_INPUT].isConnected() ? 
                      inputs[RIGHT_INPUT].getVoltage() : leftInput;
    
    // Apply soft clipping to inputs to prevent harsh distortion
    leftInput = softClip(leftInput * 0.1f); // Scale down from ±10V to ±1V range
    rightInput = softClip(rightInput * 0.1f);
    
    // Process through delay lines
    float leftOutput = leftDelay.process(leftInput);
    float rightOutput = rightDelay.process(rightInput);
    
    // Apply cross-feedback if enabled
    if (params[CROSS_FEEDBACK_PARAM].getValue() > 0.5f) {
        float crossAmount = 0.3f; // Fixed cross-feedback amount
        leftOutput += rightDelay.getDelayedSignal() * crossAmount;
        rightOutput += leftDelay.getDelayedSignal() * crossAmount;
    }
    
    // Process through pitch shifter
    if (std::abs(params[PITCH_PARAM].getValue()) > 0.01f || 
        inputs[PITCH_CV_INPUT].isConnected()) {
        leftOutput = pitchShifter.process(leftOutput, 0); // Left channel
        rightOutput = pitchShifter.process(rightOutput, 1); // Right channel
    }
    
    // Process through tape delay if enabled
    if (params[TAPE_MODE_PARAM].getValue() > 0.5f) {
        leftOutput = tapeProcessor.process(leftOutput, 0);
        rightOutput = tapeProcessor.process(rightOutput, 1);
    }
    
    // Update level meters periodically
    if (processCounter % LEVEL_UPDATE_RATE == 0) {
        updateLevelMeters(std::abs(leftInput), std::abs(rightInput));
    }
    
    // Update status lights
    updateStatusLights();
    
    // Set outputs (scale back up to ±10V range)
    outputs[LEFT_OUTPUT].setVoltage(leftOutput * 10.0f);
    outputs[RIGHT_OUTPUT].setVoltage(rightOutput * 10.0f);
}

inline json_t* CurveAndDragModule::dataToJson() {
    json_t* rootJ = json_object();
    
    // Save tuning information
    json_object_set_new(rootJ, "tuningSource", json_string(tuningSource.c_str()));
    json_object_set_new(rootJ, "tuningInfo", json_string(tuningInfo.c_str()));
    
    return rootJ;
}

inline void CurveAndDragModule::dataFromJson(json_t* rootJ) {
    // Load tuning information
    json_t* tuningSourceJ = json_object_get(rootJ, "tuningSource");
    if (tuningSourceJ) {
        tuningSource = json_string_value(tuningSourceJ);
    }
    
    json_t* tuningInfoJ = json_object_get(rootJ, "tuningInfo");
    if (tuningInfoJ) {
        tuningInfo = json_string_value(tuningInfoJ);
    }
}

// Helper method implementations
inline float CurveAndDragModule::getClampedParam(int paramId, int cvInputId, 
                                                float minVal, float maxVal, float defaultVal) {
    float paramValue = params[paramId].getValue();
    
    if (inputs[cvInputId].isConnected()) {
        float cvValue = inputs[cvInputId].getVoltage() / 10.0f; // Normalize ±10V to ±1
        paramValue += cvValue;
    }
    
    return rack::math::clamp(paramValue, minVal, maxVal);
}

inline void CurveAndDragModule::processDelayParameters(float sampleRate) {
    // Process left delay time
    float leftTimeMs = getClampedParam(TIME_L_PARAM, TIME_L_CV_INPUT, 0.0f, 1.0f) * 2000.0f;
    leftDelay.setDelayTime(leftTimeMs);
    
    // Process right delay time
    float rightTimeMs = getClampedParam(TIME_R_PARAM, TIME_R_CV_INPUT, 0.0f, 1.0f) * 2000.0f;
    rightDelay.setDelayTime(rightTimeMs);
    
    // Process feedback amounts
    float leftFeedback = getClampedParam(FEEDBACK_L_PARAM, FEEDBACK_L_CV_INPUT, 0.0f, 1.1f);
    leftDelay.setFeedback(leftFeedback);
    
    float rightFeedback = getClampedParam(FEEDBACK_R_PARAM, FEEDBACK_R_CV_INPUT, 0.0f, 1.1f);
    rightDelay.setFeedback(rightFeedback);
    
    // Process mix amounts
    float leftMix = getClampedParam(MIX_L_PARAM, MIX_L_CV_INPUT, 0.0f, 1.0f);
    leftDelay.setDryWet(leftMix);
    
    float rightMix = getClampedParam(MIX_R_PARAM, MIX_R_CV_INPUT, 0.0f, 1.0f);
    rightDelay.setDryWet(rightMix);
}

inline void CurveAndDragModule::processTapTempo(float sampleRate) {
    // Process left tap tempo
    if (leftTapTrigger.process(inputs[TAP_L_TRIGGER_INPUT].getVoltage())) {
        float tapTime = leftTapTimer.getTime();
        if (tapTime > 0.05f && tapTime < 2.0f) { // Valid tap range: 50ms to 2s
            leftDelay.setDelayTime(tapTime * 1000.0f); // Convert to ms
        }
        leftTapTimer.reset();
    }
    
    // Process right tap tempo
    if (rightTapTrigger.process(inputs[TAP_R_TRIGGER_INPUT].getVoltage())) {
        float tapTime = rightTapTimer.getTime();
        if (tapTime > 0.05f && tapTime < 2.0f) { // Valid tap range: 50ms to 2s
            rightDelay.setDelayTime(tapTime * 1000.0f); // Convert to ms
        }
        rightTapTimer.reset();
    }
    
    // Update timers
    leftTapTimer.process(1.0f / sampleRate);
    rightTapTimer.process(1.0f / sampleRate);
}

inline void CurveAndDragModule::processPitchParameters() {
    // Get pitch shift amount in cents
    float pitchCents = getClampedParam(PITCH_PARAM, PITCH_CV_INPUT, -1.0f, 1.0f) * 1200.0f;
    
    // Apply detune drift if enabled
    float detuneDrift = getClampedParam(DETUNE_DRIFT_PARAM, DETUNE_DRIFT_CV_INPUT, 0.0f, 1.0f);
    if (detuneDrift > 0.01f) {
        // Add some random drift (simplified implementation)
        static float driftPhase = 0.0f;
        driftPhase += 0.001f; // Slow drift rate
        float drift = std::sin(driftPhase) * detuneDrift * 50.0f; // ±50 cents max drift
        pitchCents += drift;
    }
    
    lastRawPitch = pitchCents;
    
    // Apply quantization if enabled
    bool quantizeEnabled = params[QUANTIZE_PARAM].getValue() > 0.5f;
    if (quantizeEnabled) {
        if (mtsClient.isMtsConnected()) {
            // Use MTS-ESP for quantization
            float midiNote = 60.0f + (pitchCents / 100.0f); // Convert cents to MIDI note
            double retuning = mtsClient.getRetuningInSemitones(midiNote);
            lastQuantizedPitch = retuning * 100.0f; // Convert back to cents
        } else {
            // Use internal scale quantization
            lastQuantizedPitch = scalaReader.quantizePitch(pitchCents);
        }
    } else {
        lastQuantizedPitch = pitchCents;
    }
    
    // Apply morph between raw and quantized pitch
    float morphAmount = getClampedParam(MORPH_PARAM, MORPH_CV_INPUT, 0.0f, 1.0f);
    float finalPitch = lastRawPitch * (1.0f - morphAmount) + lastQuantizedPitch * morphAmount;
    
    // Set pitch shifter parameters
    pitchShifter.setPitchShift(finalPitch);
}

inline void CurveAndDragModule::processTapeParameters() {
    if (params[TAPE_MODE_PARAM].getValue() < 0.5f) {
        return; // Tape mode disabled
    }
    
    // Process wow parameters
    float wowRate = getClampedParam(WOW_RATE_PARAM, WOW_RATE_CV_INPUT, 0.1f, 5.0f);
    float wowDepth = getClampedParam(WOW_DEPTH_PARAM, WOW_DEPTH_CV_INPUT, 0.0f, 1.0f);
    tapeProcessor.setWowParameters(wowRate, wowDepth);
    
    // Process flutter parameters
    float flutterRate = getClampedParam(FLUTTER_RATE_PARAM, FLUTTER_RATE_CV_INPUT, 1.0f, 5.0f);
    float flutterDepth = params[FLUTTER_DEPTH_PARAM].getValue();
    tapeProcessor.setFlutterParameters(flutterRate, flutterDepth);
    
    // Process saturation
    float saturation = getClampedParam(SATURATION_PARAM, SATURATION_CV_INPUT, 0.0f, 1.0f);
    tapeProcessor.setSaturation(saturation);
    
    // Process noise
    float noiseAmount = getClampedParam(NOISE_AMOUNT_PARAM, NOISE_AMOUNT_CV_INPUT, 0.0f, 0.1f);
    tapeProcessor.setNoiseAmount(noiseAmount);
}

inline void CurveAndDragModule::processScaleSelection() {
    int scaleSelect = static_cast<int>(params[SCALE_SELECT_PARAM].getValue());
    
    switch (scaleSelect) {
        case 0: // 12-TET
            scalaReader.setDefaultScale();
            tuningSource = "12-TET";
            tuningInfo = "Equal Temperament";
            break;
        case 1: // Just Intonation
            scalaReader.setJustIntonationScale();
            tuningSource = "Just";
            tuningInfo = "Just Intonation";
            break;
        case 2: // MTS-ESP
            if (mtsClient.isMtsConnected()) {
                tuningSource = "MTS-ESP";
                tuningInfo = mtsClient.getMtsTuningName();
            } else {
                tuningSource = "MTS-ESP";
                tuningInfo = "Not Connected";
            }
            break;
    }
}

inline void CurveAndDragModule::updateLevelMeters(float leftInput, float rightInput) {
    // Smooth level values for display
    leftLevelSmooth += (leftInput - leftLevelSmooth) * LEVEL_SMOOTH_RATE;
    rightLevelSmooth += (rightInput - rightLevelSmooth) * LEVEL_SMOOTH_RATE;
    
    // Update level lights (5 lights per channel)
    for (int i = 0; i < 5; i++) {
        float threshold = (i + 1) * 0.2f; // 20%, 40%, 60%, 80%, 100%
        lights[LEVEL_LIGHTS_L_START + i].setBrightness(leftLevelSmooth > threshold ? 1.0f : 0.0f);
        lights[LEVEL_LIGHTS_R_START + i].setBrightness(rightLevelSmooth > threshold ? 1.0f : 0.0f);
    }
}

inline void CurveAndDragModule::updateStatusLights() {
    // Update sync lights
    lights[SYNC_L_LIGHT].setBrightness(params[SYNC_L_PARAM].getValue());
    lights[SYNC_R_LIGHT].setBrightness(params[SYNC_R_PARAM].getValue());
    
    // Update cross-feedback light
    lights[CROSS_FEEDBACK_LIGHT].setBrightness(params[CROSS_FEEDBACK_PARAM].getValue());
    
    // Update quantize light
    lights[QUANTIZE_LIGHT].setBrightness(params[QUANTIZE_PARAM].getValue());
    
    // Update tape mode light
    lights[TAPE_MODE_LIGHT].setBrightness(params[TAPE_MODE_PARAM].getValue());
    
    // Update MTS-ESP active light
    lights[MTS_ACTIVE_LIGHT].setBrightness(mtsClient.isMtsConnected() ? 1.0f : 0.1f);
}

} // namespace CurveAndDrag
