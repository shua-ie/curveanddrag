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
 * @brief Main CurveAndDrag module class
 * 
 * A stereo delay and pitch shifter with microtonal support via MTS-ESP.
 * Features tape-style delay effects, cross-feedback, and comprehensive
 * CV control over all parameters.
 */
class CurveAndDragModule : public Module {
public:
    /**
     * @brief Parameter IDs for the CurveAndDrag module - v2.8.0 Complete
     * Total: 35 Parameters
     */
    enum ParamIds {
        // Core delay parameters (6)
        TIME_L_PARAM,
        TIME_R_PARAM,
        FEEDBACK_L_PARAM,
        FEEDBACK_R_PARAM,
        MIX_L_PARAM,
        MIX_R_PARAM,
        
        // Sync and routing parameters (4)
        SYNC_L_PARAM,
        SYNC_R_PARAM,
        SUBDIV_L_PARAM,         // NEW: Left subdivision select
        SUBDIV_R_PARAM,         // NEW: Right subdivision select
        
        // Cross-feedback and gain (3)
        CROSS_FEEDBACK_PARAM,
        INPUT_GAIN_PARAM,       // NEW: Input gain control
        OUTPUT_GAIN_PARAM,      // NEW: Output gain control
        
        // Pitch shifting parameters (8) - FIXED: Added DETUNE_L/R
        PITCH_PARAM,
        DETUNE_DRIFT_PARAM,
        DETUNE_L_PARAM,         // NEW: Left channel detune 
        DETUNE_R_PARAM,         // NEW: Right channel detune
        QUANTIZE_PARAM,
        MORPH_PARAM,
        PITCH_MODE_PARAM,       // NEW: BBD/H910/Varispeed/Hybrid mode select
        CHARACTER_PARAM,        // NEW: Character/vintage modeling
        
        // Scale and tuning (2)
        SCALE_SELECT_PARAM,
        MTS_ENABLE_PARAM,
        
        // Tape delay parameters (14)
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
        HEAD_SELECT_PARAM,      // NEW: Multi-head configuration
        AGING_PARAM,            // NEW: Tape aging amount
        INSTABILITY_PARAM,      // NEW: Tape instability
        
        NUM_PARAMS
    };

    /**
     * @brief Input IDs for the CurveAndDrag module - v2.8.0 Complete
     * Total: 21 CV Inputs (+ 2 audio + 2 tap triggers = 25 total inputs)
     */
    enum InputIds {
        // Audio inputs (2)
        LEFT_INPUT,
        RIGHT_INPUT,
        
        // Core delay CV inputs (6)
        TIME_L_CV_INPUT,
        TIME_R_CV_INPUT,
        FEEDBACK_L_CV_INPUT,
        FEEDBACK_R_CV_INPUT,
        MIX_L_CV_INPUT,
        MIX_R_CV_INPUT,
        
        // Global modulation CV (2)
        TIME_MOD_INPUT,         // NEW: Global time modulation
        FEEDBACK_MOD_INPUT,     // NEW: Global feedback modulation
        
        // Trigger inputs (2)
        TAP_L_TRIGGER_INPUT,
        TAP_R_TRIGGER_INPUT,
        
        // Pitch shift CV inputs (5) - FIXED: Added DETUNE_L/R_CV
        PITCH_CV_INPUT,
        DETUNE_DRIFT_CV_INPUT,
        DETUNE_L_CV_INPUT,      // NEW: Left detune CV
        DETUNE_R_CV_INPUT,      // NEW: Right detune CV  
        MORPH_CV_INPUT,
        
        // Gain CV inputs (3)
        INPUT_GAIN_CV_INPUT,    // NEW: Input gain CV
        OUTPUT_GAIN_CV_INPUT,   // NEW: Output gain CV
        CHARACTER_CV_INPUT,     // NEW: Character morph CV
        
        // Tape delay CV inputs (6)
        WOW_RATE_CV_INPUT,
        WOW_DEPTH_CV_INPUT,
        FLUTTER_RATE_CV_INPUT,
        FLUTTER_DEPTH_CV_INPUT, // NEW: Flutter depth CV
        SATURATION_CV_INPUT,
        TAPE_NOISE_CV_INPUT,    // NEW: Tape noise CV
        NOISE_AMOUNT_CV_INPUT,  // NEW: Noise amount CV
        AGING_CV_INPUT,         // NEW: Aging CV
        INSTABILITY_CV_INPUT,   // NEW: Instability CV
        HEAD_SELECT_CV_INPUT,   // NEW: Head selection CV
        
        NUM_INPUTS
    };

    /**
     * @brief Output IDs for the CurveAndDrag module - v2.8.0 Complete
     */
    enum OutputIds {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,
        WET_LEFT_OUTPUT,        // NEW: Wet-only left output
        WET_RIGHT_OUTPUT,       // NEW: Wet-only right output
        NUM_OUTPUTS
    };

    /**
     * @brief Light IDs for the CurveAndDrag module - v2.8.0 Complete
     */
    enum LightIds {
        // Status lights
        SYNC_L_LIGHT,
        SYNC_R_LIGHT,
        CROSS_FEEDBACK_LIGHT,
        QUANTIZE_LIGHT,
        TAPE_MODE_LIGHT,
        MTS_ACTIVE_LIGHT,
        MORPH_LIGHT,            // NEW: Morph active indicator
        
        // Level meter lights (5 segments per channel)
        LEVEL_LIGHTS_L_START,
        LEVEL_LIGHTS_L_END = LEVEL_LIGHTS_L_START + 4,
        LEVEL_LIGHTS_R_START,
        LEVEL_LIGHTS_R_END = LEVEL_LIGHTS_R_START + 4,
        
        NUM_LIGHTS
    };

    /**
     * @brief Pitch shifting modes enum
     */
    enum PitchMode {
        PITCH_BBD = 0,          // Bucket Brigade style
        PITCH_H910,             // Harmonizer style
        PITCH_VARISPEED,        // Tape varispeed style
        PITCH_HYBRID            // Hybrid algorithm
    };

    /**
     * @brief Musical subdivision types for tempo sync
     */
    enum SubdivisionType {
        SUBDIVISION_1_1 = 0,    // 1/1 - Whole note
        SUBDIVISION_1_2,        // 1/2 - Half note  
        SUBDIVISION_1_4,        // 1/4 - Quarter note
        SUBDIVISION_1_8,        // 1/8 - Eighth note
        SUBDIVISION_1_8T,       // 1/8T - Eighth triplet
        SUBDIVISION_1_16        // 1/16 - Sixteenth note
    };

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
     * @brief Called when sample rate changes
     */
    void onSampleRateChange() override;

    /**
     * @brief Main audio processing function
     */
    void process(const ProcessArgs& args) override;

    /**
     * @brief Load module state from JSON
     */
    json_t* dataToJson() override;

    /**
     * @brief Save module state to JSON
     */
    void dataFromJson(json_t* rootJ) override;

    // Public member variables for GUI access
    std::string tuningSource = "12-TET";
    std::string tuningInfo = "Equal Temperament";
    float lastRawPitch = 0.0f;
    float lastQuantizedPitch = 0.0f;

    // Public access to ScalaReader for GUI
    ScalaReader scalaReader;

    /**
     * @brief Get subdivision time in milliseconds for tempo sync
     * @param subdivision The subdivision type
     * @param beatDurationMs Beat duration in milliseconds  
     * @return Time in milliseconds for the subdivision
     */
    float getSubdivisionTimeMs(SubdivisionType subdivision, float beatDurationMs);
    
    /**
     * @brief Get subdivision name for display
     * @param subdivision The subdivision type
     * @return String representation of the subdivision
     */
    std::string getSubdivisionName(SubdivisionType subdivision);

    // ===== NEW MEMBER VARIABLES FOR ENHANCED FUNCTIONALITY =====
    float lastDetuneL = 0.0f;   // Stored left detune for independent processing  
    float lastDetuneR = 0.0f;   // Stored right detune for independent processing
    SubdivisionType currentLeftSubdivision = SUBDIVISION_1_4;
    SubdivisionType currentRightSubdivision = SUBDIVISION_1_4;
    int currentScaleIndex = 0;

private:
    // Audio processing components
    DelayLine leftDelay;
    DelayLine rightDelay;
    PitchShifter pitchShifter;
    TapeDelayProcessor tapeProcessor;
    MTSESPClient mtsClient;

    // Timing and trigger components
    dsp::SchmittTrigger leftTapTrigger;
    dsp::SchmittTrigger rightTapTrigger;
    dsp::Timer leftTapTimer;
    dsp::Timer rightTapTimer;

    // Level meter smoothing
    float leftLevelSmooth = 0.0f;
    float rightLevelSmooth = 0.0f;
    static constexpr float LEVEL_SMOOTH_RATE = 0.01f;

    // CV modulation storage
    float pitchCVModulation = 0.0f;
    float outputGainModulation = 0.0f;
    float timeCVGlobal = 0.0f;
    float feedbackCVGlobal = 0.0f;

    // Process rate limiting
    int processCounter = 0;
    static constexpr int MTS_POLL_RATE = 1024; // Poll MTS every N samples
    static constexpr int LEVEL_UPDATE_RATE = 64; // Update levels every N samples
    static constexpr int DISPLAY_UPDATE_RATE = 512; // Update displays every N samples

    // Tempo detection
    float detectedBPM = 120.0f;
    float lastTapTime = 0.0f;

    // ===== PRIVATE METHODS =====

    /**
     * @brief Process all 21 CV inputs for complete automation
     */
    void processAllCVInputs();

    /**
     * @brief Update parameter displays with proper subdivision names
     */
    void updateParameterDisplays();

    /**
     * @brief Process tempo sync with musical subdivisions
     */
    void processTempo();

    /**
     * @brief Process tape mode with full stereo functionality
     */
    void processTapeMode();

    /**
     * @brief Process delay parameters with CV modulation
     */
    void processDelayParameters(float sampleRate);

    /**
     * @brief Process tap tempo detection and application
     */
    void processTapTempo(float sampleRate);

    /**
     * @brief Process pitch shifting parameters with CV
     */
    void processPitchParameters();

    /**
     * @brief Process tape delay parameters with CV
     */
    void processTapeParameters();

    /**
     * @brief Process scale selection and MTS-ESP integration
     */
    void processScaleSelection();

    /**
     * @brief Update 5-segment level meters for both channels
     * 
     * @param leftInput Left channel input level
     * @param rightInput Right channel input level
     */
    void updateLevelMeters(float leftInput, float rightInput);

    /**
     * @brief Update all status indicator lights
     */
    void updateStatusLights();

    // ===== UTILITY METHODS =====

    /**
     * @brief Get parameter value with optional CV modulation
     * 
     * @param paramId Parameter ID
     * @param cvInputId CV input ID (-1 for no CV)
     * @param minVal Minimum parameter value
     * @param maxVal Maximum parameter value
     * @param defaultVal Default value if CV not connected
     * @return Clamped parameter value with CV modulation
     */
    float getClampedParam(int paramId, int cvInputId, float minVal, float maxVal, float defaultVal = 0.0f);

    /**
     * @brief Quantize pitch using MTS-ESP
     * 
     * @param pitchCents Raw pitch in cents
     * @return Quantized pitch in cents
     */
    float quantizePitchMTS(float pitchCents);

    /**
     * @brief Quantize pitch using loaded Scala file
     * 
     * @param pitchCents Raw pitch in cents
     * @return Quantized pitch in cents
     */
    float quantizePitchScala(float pitchCents);

    /**
     * @brief Quantize pitch using built-in scales
     * 
     * @param pitchCents Raw pitch in cents
     * @param scaleIndex Scale selection index
     * @return Quantized pitch in cents
     */
    float quantizePitchBuiltIn(float pitchCents, int scaleIndex);

    /**
     * @brief Apply soft clipping to prevent harsh distortion
     */
    inline float softClip(float input) {
        return std::tanh(input * 0.7f);
    }

    /**
     * @brief Convert milliseconds to samples at current sample rate
     */
    inline float msToSamples(float ms, float sampleRate) {
        return (ms / 1000.0f) * sampleRate;
    }

    /**
     * @brief Convert samples to milliseconds at current sample rate
     */
    inline float samplesToMs(float samples, float sampleRate) {
        return (samples / sampleRate) * 1000.0f;
    }

    /**
     * @brief Get tempo subdivision multiplier
     */
    inline float getSubdivisionMultiplier(SubdivisionType subdivType) {
        switch (subdivType) {
            case SUBDIVISION_1_1: return 1.0f;
            case SUBDIVISION_1_2: return 0.5f;
            case SUBDIVISION_1_4: return 0.25f;
            case SUBDIVISION_1_8: return 0.125f;
            case SUBDIVISION_1_8T: return 0.125f / 1.5f; // 1/8 triplet
            case SUBDIVISION_1_16: return 0.0625f;
            default: return 0.25f;
        }
    }

    /**
     * @brief Get pitch mode display name
     */
    inline std::string getPitchModeName(PitchMode mode) {
        switch (mode) {
            case PITCH_BBD: return "BBD";
            case PITCH_H910: return "H910";
            case PITCH_VARISPEED: return "Varispeed";
            case PITCH_HYBRID: return "Hybrid";
            default: return "BBD";
        }
    }
};

} // namespace CurveAndDrag
