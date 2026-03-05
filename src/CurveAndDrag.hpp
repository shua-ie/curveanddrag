#pragma once
#include <rack.hpp>
#include <array>
#include <cstdint>
#include "plugin.hpp"
#include "DelayLine.hpp"
#include "ScalaReader.hpp"
#include "TapeDelayProcessor.hpp"
#include "MTS_ESP.hpp"
#include "FeedbackProcessor.hpp"
#include "BBDCompander.hpp"
#include "SpectralPitchShifter.hpp"

using namespace rack;

namespace CurveAndDrag {

/**
 * @brief Main CurveAndDrag module class — v1.0
 *
 * A stereo delay and pitch shifter with microtonal support via MTS-ESP.
 * Features shimmer/cascade architecture with pitch-in-feedback-loop,
 * tape-style delay effects, cross-feedback, frequency shifting, freeze,
 * and comprehensive CV control over all parameters.
 */
class CurveAndDragModule : public Module {
public:
    /**
     * @brief Pitch placement modes
     */
    enum PitchPlacement {
        PITCH_PRE_LOOP = 0,   // Classic: pitch before delay (no shimmer)
        PITCH_IN_LOOP         // Shimmer: pitch inside feedback loop
    };

    /**
     * @brief Pitch step direction for shimmer
     */
    enum PitchDirection {
        PITCH_DIR_UP = 0,     // +N semitones per repeat
        PITCH_DIR_DOWN,       // -N semitones per repeat
        PITCH_DIR_ALTERNATE   // Up/down alternation
    };

    /**
     * @brief Parameter IDs — v1.0 Complete
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
        SUBDIV_L_PARAM,
        SUBDIV_R_PARAM,

        // Cross-feedback and gain (3)
        CROSS_FEEDBACK_PARAM,
        INPUT_GAIN_PARAM,
        OUTPUT_GAIN_PARAM,

        // Pitch shifting parameters (8) — original
        PITCH_PARAM,
        DETUNE_DRIFT_PARAM,
        DETUNE_L_PARAM,
        DETUNE_R_PARAM,
        QUANTIZE_PARAM,
        MORPH_PARAM,
        PITCH_MODE_PARAM,
        CHARACTER_PARAM,

        // Scale and tuning (2)
        SCALE_SELECT_PARAM,
        MTS_ENABLE_PARAM,

        // Tape delay parameters (14) — original
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
        HEAD_SELECT_PARAM,
        AGING_PARAM,
        INSTABILITY_PARAM,

        // === SHIMMER + FEEDBACK PARAMETERS ===

        // Shimmer / Feedback architecture (6)
        PITCH_PLACEMENT_PARAM,    // Pre-loop vs In-loop toggle
        SHIMMER_PITCH_PARAM,      // In-loop pitch step in semitones (±24)
        SHIMMER_MIX_PARAM,        // Wet/dry of shimmer pitch in feedback
        PITCH_DIRECTION_PARAM,    // Up/Down/Alternate
        SHIMMER_QUANTIZE_PARAM,   // Quantize per-iteration pitch to scale

        // Feedback path processing (6)
        FB_LP_FREQ_PARAM,        // Feedback LP filter frequency
        FB_HP_FREQ_PARAM,        // Feedback HP filter frequency
        FB_TILT_PARAM,           // Feedback tilt EQ (-1 dark, +1 bright)
        FB_DRIVE_PARAM,          // Feedback path saturation
        FB_COMP_PARAM,           // Feedback path compression

        // Ducking (2)
        DUCK_AMOUNT_PARAM,       // How much feedback ducks when input present
        DUCK_RELEASE_PARAM,      // Duck release time

        // Pitch drift (2)
        PITCH_DRIFT_PARAM,       // Per-iteration drift amount in cents
        DRIFT_RATE_PARAM,        // Drift speed

        // Freeze (1)
        FREEZE_PARAM,            // Freeze/infinite hold toggle

        // Frequency shifting - Ghost mode (2)
        FREQ_SHIFT_PARAM,        // Hz shift amount (±50Hz)
        FREQ_SHIFT_MODE_PARAM,   // Enable freq shift mode

        // Independent L/R pitch (2)
        PITCH_L_PARAM,           // Left-only pitch offset (±12 semitones)
        PITCH_R_PARAM,           // Right-only pitch offset (±12 semitones)

        // Pitch envelope follower (2)
        PITCH_ENV_AMOUNT_PARAM,  // Envelope → pitch modulation depth
        PITCH_ENV_SPEED_PARAM,   // Envelope follower speed

        // Reverse-granular (2)
        REVERSE_GRAIN_PARAM,     // Reverse grain mix
        REVERSE_GRAIN_SIZE_PARAM,// Grain size (ms)

        // Bloom mode (2)
        BLOOM_AMOUNT_PARAM,      // Detune/chorus amount in feedback
        BLOOM_RATE_PARAM,        // Modulation rate

        // Time stretch — spectral mode only (1)
        TIME_STRETCH_PARAM,      // Time stretch ratio (0.5x - 2.0x)

        NUM_PARAMS
    };

    /**
     * @brief Input IDs — v1.0 Complete
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
        TIME_MOD_INPUT,
        FEEDBACK_MOD_INPUT,

        // Trigger inputs (2)
        TAP_L_TRIGGER_INPUT,
        TAP_R_TRIGGER_INPUT,

        // Pitch shift CV inputs (5) — original
        PITCH_CV_INPUT,
        DETUNE_DRIFT_CV_INPUT,
        DETUNE_L_CV_INPUT,
        DETUNE_R_CV_INPUT,
        MORPH_CV_INPUT,

        // Gain CV inputs (3)
        INPUT_GAIN_CV_INPUT,
        OUTPUT_GAIN_CV_INPUT,
        CHARACTER_CV_INPUT,

        // Tape delay CV inputs (10) — original
        WOW_RATE_CV_INPUT,
        WOW_DEPTH_CV_INPUT,
        FLUTTER_RATE_CV_INPUT,
        FLUTTER_DEPTH_CV_INPUT,
        SATURATION_CV_INPUT,
        TAPE_NOISE_CV_INPUT,
        NOISE_AMOUNT_CV_INPUT,
        AGING_CV_INPUT,
        INSTABILITY_CV_INPUT,
        HEAD_SELECT_CV_INPUT,

        // === SHIMMER + FEEDBACK CV INPUTS ===
        SHIMMER_PITCH_CV_INPUT,  // Shimmer pitch step CV
        FB_LP_FREQ_CV_INPUT,     // Feedback LP frequency CV
        FB_HP_FREQ_CV_INPUT,     // Feedback HP frequency CV
        DUCK_AMOUNT_CV_INPUT,    // Duck amount CV
        FREQ_SHIFT_CV_INPUT,     // Frequency shift CV
        PITCH_L_CV_INPUT,        // Left pitch offset CV
        PITCH_R_CV_INPUT,        // Right pitch offset CV
        FREEZE_CV_INPUT,         // Freeze gate input
        PITCH_ENV_CV_INPUT,      // Pitch envelope amount CV
        REVERSE_GRAIN_CV_INPUT,  // Reverse grain mix CV
        BLOOM_CV_INPUT,          // Bloom amount CV

        // V/Oct input for MIDI-driven pitch
        PITCH_VOCT_INPUT,        // 1V/Oct pitch control

        // Time stretch CV
        TIME_STRETCH_CV_INPUT,   // Time stretch CV input

        NUM_INPUTS
    };

    /**
     * @brief Output IDs — v1.0 Complete
     */
    enum OutputIds {
        LEFT_OUTPUT,
        RIGHT_OUTPUT,
        WET_LEFT_OUTPUT,
        WET_RIGHT_OUTPUT,
        NUM_OUTPUTS
    };

    /**
     * @brief Light IDs — v1.0 Complete
     */
    enum LightIds {
        SYNC_L_LIGHT,
        SYNC_R_LIGHT,
        CROSS_FEEDBACK_LIGHT,
        QUANTIZE_LIGHT,
        TAPE_MODE_LIGHT,
        MTS_ACTIVE_LIGHT,
        MORPH_LIGHT,

        // Level meter lights (5 segments per channel, 2 IDs each for GreenRedLight)
        LEVEL_LIGHTS_L_START,
        LEVEL_LIGHTS_L_END = LEVEL_LIGHTS_L_START + 9, // 5 * 2 - 1
        LEVEL_LIGHTS_R_START,
        LEVEL_LIGHTS_R_END = LEVEL_LIGHTS_R_START + 9, // 5 * 2 - 1

        // Shimmer + feedback lights
        SHIMMER_LIGHT,
        FREEZE_LIGHT,
        FREQ_SHIFT_LIGHT,
        REVERSE_LIGHT,
        BLOOM_LIGHT,

        NUM_LIGHTS
    };

    /**
     * @brief Pitch shifting modes enum
     */
    enum PitchMode {
        PITCH_LOFI = 0,
        PITCH_H910,
        PITCH_VARISPEED,
        PITCH_HYBRID,
        PITCH_SPECTRAL
    };

    /**
     * @brief Musical subdivision types for tempo sync
     */
    enum SubdivisionType {
        SUBDIVISION_1_1 = 0,
        SUBDIVISION_1_2,
        SUBDIVISION_1_4,
        SUBDIVISION_1_8,
        SUBDIVISION_1_8T,
        SUBDIVISION_1_16
    };

    CurveAndDragModule();
    ~CurveAndDragModule() = default;

    void onReset() override;
    void onSampleRateChange() override;
    void process(const ProcessArgs& args) override;
    json_t* dataToJson() override;
    void dataFromJson(json_t* rootJ) override;

    // Public member variables for GUI access
    std::string tuningSource = "12-TET";
    std::string tuningInfo = "Equal Temperament";
    float lastRawPitch = 0.0f;
    float lastQuantizedPitch = 0.0f;

    ScalaReader scalaReader;

    float getSubdivisionTimeMs(SubdivisionType subdivision, float beatDurationMs);
    std::string getSubdivisionName(SubdivisionType subdivision);

    float lastDetuneL = 0.0f;
    float lastDetuneR = 0.0f;
    SubdivisionType currentLeftSubdivision = SUBDIVISION_1_4;
    SubdivisionType currentRightSubdivision = SUBDIVISION_1_4;
    int currentScaleIndex = 0;

    // Public state for GUI
    bool isShimmerActive() const { return pitchPlacement == PITCH_IN_LOOP; }
    bool isFreezeActive() const { return feedbackProc.freezeEnabled; }

private:
    // Audio processing components
    DelayLine leftDelay;
    DelayLine rightDelay;
    TapeDelayProcessor tapeProcessor;
    MTSESPClient mtsClient;
    FeedbackProcessor feedbackProc;

    // Shimmer state
    PitchPlacement pitchPlacement = PITCH_PRE_LOOP;

    // Timing and trigger components
    dsp::SchmittTrigger leftTapTrigger;
    dsp::SchmittTrigger rightTapTrigger;
    dsp::SchmittTrigger freezeTrigger;
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
    uint64_t processCounter = 0;
    static constexpr int MTS_POLL_RATE = 1024;
    static constexpr int LEVEL_UPDATE_RATE = 64;
    static constexpr int DISPLAY_UPDATE_RATE = 512;

    // Tempo detection
    float detectedBPM = 120.0f;
    float lastTapTime = 0.0f;

    // Pitch smoothing state
    float smoothedBasePitch = 0.0f;
    float smoothedDetuneL = 0.0f;
    float smoothedDetuneR = 0.0f;
    float smoothedDrift = 0.0f;

    // BBD pitch shifter state
    std::array<float, 8192> leftBBDBuffer = {};
    std::array<float, 8192> rightBBDBuffer = {};
    int bbdIndex = 0;
    float leftBBDPhase = 0.0f;
    float rightBBDPhase = 0.0f;

    // H910 pitch shifter state
    std::array<float, 4096> leftH910Buffer = {};
    std::array<float, 4096> rightH910Buffer = {};
    int h910Index = 0;
    float leftGrainPhase = 0.0f;
    float rightGrainPhase = 0.0f;

    // Varispeed pitch shifter state
    std::array<float, 16384> leftVarBuffer = {};
    std::array<float, 16384> rightVarBuffer = {};
    float leftVarReadPos = 0.0f;
    float rightVarReadPos = 0.0f;
    int varWritePos = 0;

    // Hybrid pitch shifter state
    std::array<float, 8192> leftHybridBuffer = {};
    std::array<float, 8192> rightHybridBuffer = {};
    int hybridIndex = 0;
    float leftHybridPhase = 0.0f;
    float rightHybridPhase = 0.0f;

    // Cross-feedback state
    float prevLeftDelayed = 0.0f;
    float prevRightDelayed = 0.0f;
    float leftCrossFilter = 0.0f;
    float rightCrossFilter = 0.0f;

    // Anti-alias 4th-order filter state (cascaded biquads for 24dB/oct)
    rack::dsp::BiquadFilter antiAliasFilterL1;
    rack::dsp::BiquadFilter antiAliasFilterR1;
    rack::dsp::BiquadFilter antiAliasFilterL2;
    rack::dsp::BiquadFilter antiAliasFilterR2;
    float lastAntiAliasRatio = 0.0f;

    // BBD emulation filter state
    float bbdLpfL = 0.0f;
    float bbdLpfR = 0.0f;

    // BBD clock feedthrough and charge transfer state
    float bbdClockPhaseL = 0.0f;
    float bbdClockPhaseR = 0.0f;
    float bbdPrevSampleL = 0.0f;
    float bbdPrevSampleR = 0.0f;

    // H910 clock drift state
    float h910DriftL = 0.0f;
    float h910DriftR = 0.0f;
    float h910DriftPhaseL = 0.0f;
    float h910DriftPhaseR = 0.0f;

    // 2nd-order DC blocker (Julius O. Smith design)
    struct DCBlocker2 {
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        float R = 0.995f;
        DCBlocker2(float r = 0.995f) : R(r) {}
        float process(float x) {
            float y = x - 2*x1 + x2 + 2*R*y1 - R*R*y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y;
            return y;
        }
        void reset() { x1=x2=y1=y2=0; }
    };

    DCBlocker2 dcBlockerL;
    DCBlocker2 dcBlockerR;
    DCBlocker2 fbDcBlockerL; // Feedback path DC blocker (shimmer mode)
    DCBlocker2 fbDcBlockerR;

    // Display update state
    float lastLeftSubdiv = -1.0f;
    float lastRightSubdiv = -1.0f;

    // === SHIMMER + EFFECTS STATE ===

    // Reverse-granular state (Crystallizer-style)
    std::array<float, 16384> reverseGrainBufL = {};
    std::array<float, 16384> reverseGrainBufR = {};
    int reverseGrainWritePos = 0;
    float reverseGrainPhaseL = 0.0f;
    float reverseGrainPhaseR = 0.0f;

    // Bloom state (chorus in feedback)
    float bloomPhaseL = 0.0f;
    float bloomPhaseR = 0.0f;

    // Pitch envelope follower state
    EnvelopeFollower pitchEnvFollowerL;
    EnvelopeFollower pitchEnvFollowerR;

    // BBD compander (Colour Copy-style NE570 modeling)
    BBDCompander bbdCompanderCompress; // Compress before BBD
    BBDCompander bbdCompanderExpand;   // Expand after BBD

    // Phase-locking pitch shifter (Laroche-Dolson)
    PhaseLockingPitchShifter phaseLockShifterL;
    PhaseLockingPitchShifter phaseLockShifterR;

    // Spectral pitch shifter (FFT phase vocoder with formant preservation)
    SpectralPitchShifter spectralShifterL;
    SpectralPitchShifter spectralShifterR;

    // Per-head DSP parameters (context menu only, not module params)
    float headPitchSemitones[4] = {0, 0, 0, 0};
    float headFilterCutoff[4] = {8000, 8000, 8000, 8000};
    int headFilterMode[4] = {0, 0, 0, 0}; // SVFilter::OFF
    float headLevel[4] = {1, 1, 1, 1};
    float headPan[4] = {0.5f, 0.5f, 0.5f, 0.5f};

    // Bloom modulated delay buffers
    static constexpr int BLOOM_BUFFER_SIZE = 8192;
    std::array<float, BLOOM_BUFFER_SIZE> bloomDelayBufL = {};
    std::array<float, BLOOM_BUFFER_SIZE> bloomDelayBufR = {};
    int bloomWritePosL = 0;
    int bloomWritePosR = 0;

    // Freeze state
    bool freezeActive = false;


    // TPDF dither state
    uint32_t ditherSeed = 12345;

    // Input level tracking for ducking
    float inputLevelL = 0.0f;
    float inputLevelR = 0.0f;

    // === PRIVATE METHODS ===

    void processAllCVInputs();
    void updateParameterDisplays();
    void processTempo();
    void processTapeMode();
    void processDelayParameters(float sampleRate);
    void processTapTempo(float sampleRate);
    void processPitchParameters();
    void processTapeParameters();
    void processScaleSelection();
    void updateLevelMeters(float leftInput, float rightInput);
    void updateStatusLights();

    /**
     * Apply pitch shifting to a signal (used both pre-loop and in-loop)
     * @param leftIn Left channel input
     * @param rightIn Right channel input
     * @param leftOut Left channel output (pitch shifted)
     * @param rightOut Right channel output (pitch shifted)
     * @param leftPitchCents Left pitch in cents
     * @param rightPitchCents Right pitch in cents
     * @param sampleRate Current sample rate
     */
    void applyPitchShift(float leftIn, float rightIn,
                         float& leftOut, float& rightOut,
                         float leftPitchCents, float rightPitchCents,
                         float sampleRate);

    /**
     * Process reverse-granular effect (Crystallizer-style)
     */
    float processReverseGrain(float input, int channel, float grainSize, float sampleRate);

    /**
     * Process bloom/chorus in feedback path
     */
    float processBloom(float input, int channel, float amount, float rate, float sampleRate);

    /**
     * Quantize pitch in cents to the current scale
     */
    float quantizePitchToScale(float pitchCents);

    // Utility methods
    float getClampedParam(int paramId, int cvInputId, float minVal, float maxVal, float defaultVal = 0.0f);
    float quantizePitchMTS(float pitchCents);
    float quantizePitchScala(float pitchCents);
    float quantizePitchBuiltIn(float pitchCents, int scaleIndex);

    inline float softClip(float x) {
        return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
    }

    inline float msToSamples(float ms, float sampleRate) {
        return (ms / 1000.0f) * sampleRate;
    }

    inline float samplesToMs(float samples, float sampleRate) {
        return (samples / sampleRate) * 1000.0f;
    }

    inline float getSubdivisionMultiplier(SubdivisionType subdivType) {
        switch (subdivType) {
            case SUBDIVISION_1_1: return 1.0f;
            case SUBDIVISION_1_2: return 0.5f;
            case SUBDIVISION_1_4: return 0.25f;
            case SUBDIVISION_1_8: return 0.125f;
            case SUBDIVISION_1_8T: return 0.125f / 1.5f;
            case SUBDIVISION_1_16: return 0.0625f;
            default: return 0.25f;
        }
    }

    inline std::string getPitchModeName(PitchMode mode) {
        switch (mode) {
            case PITCH_LOFI: return "Lo-Fi";
            case PITCH_H910: return "H910";
            case PITCH_VARISPEED: return "Varispeed";
            case PITCH_HYBRID: return "Hybrid";
            case PITCH_SPECTRAL: return "Spectral";
            default: return "Lo-Fi";
        }
    }
};

} // namespace CurveAndDrag
