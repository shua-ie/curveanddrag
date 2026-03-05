// ===== CRITICAL FIX: Math constants for Windows compilation - MUST BE FIRST =====
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

#include "CurveAndDrag.hpp"
#include "plugin.hpp"
#include "CurveAndDragWidget.hpp"
#include <rack.hpp>
#include <algorithm>
#include <cmath>
#ifdef __SSE__
#include <xmmintrin.h>
#endif
#ifdef __SSE3__
#include <pmmintrin.h>
#endif

namespace CurveAndDrag {

// ===== CONSTRUCTOR - v1.0 Complete =====
CurveAndDragModule::CurveAndDragModule() {
    config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

    // Configure core delay parameters (6)
    configParam(TIME_L_PARAM, 0.0f, 1.0f, 0.1f, "Left Delay Time", " ms", 0.0f, 2000.0f);
    configParam(TIME_R_PARAM, 0.0f, 1.0f, 0.1f, "Right Delay Time", " ms", 0.0f, 2000.0f);
    configParam(FEEDBACK_L_PARAM, 0.0f, 1.1f, 0.3f, "Left Feedback", "%", 0.0f, 100.0f);
    configParam(FEEDBACK_R_PARAM, 0.0f, 1.1f, 0.3f, "Right Feedback", "%", 0.0f, 100.0f);
    configParam(MIX_L_PARAM, 0.0f, 1.0f, 0.5f, "Left Dry/Wet Mix", "%", 0.0f, 100.0f);
    configParam(MIX_R_PARAM, 0.0f, 1.0f, 0.5f, "Right Dry/Wet Mix", "%", 0.0f, 100.0f);

    // Configure sync and subdivision parameters (4)
    configParam(SYNC_L_PARAM, 0.0f, 1.0f, 0.0f, "Left Channel Tempo Sync");
    configParam(SYNC_R_PARAM, 0.0f, 1.0f, 0.0f, "Right Channel Tempo Sync");
    configParam(SUBDIV_L_PARAM, 0.0f, 5.0f, 2.0f, "Left Channel Subdivision");
    configParam(SUBDIV_R_PARAM, 0.0f, 5.0f, 2.0f, "Right Channel Subdivision");

    // Configure cross-feedback and gain parameters (3)
    configParam(CROSS_FEEDBACK_PARAM, 0.0f, 1.0f, 0.0f, "Cross-Feedback Amount", "%", 0.0f, 100.0f);
    configParam(INPUT_GAIN_PARAM, 0.0f, 2.0f, 1.0f, "Input Gain", "x", 0.0f, 2.0f);
    configParam(OUTPUT_GAIN_PARAM, 0.0f, 2.0f, 1.0f, "Output Gain", "x", 0.0f, 2.0f);

    // Configure pitch shifting parameters (8)
    configParam(PITCH_PARAM, -2.0f, 2.0f, 0.0f, "Main Pitch Shift: ±2 octaves", " cents", 0.0f, 1200.0f);
    configParam(DETUNE_DRIFT_PARAM, 0.0f, 1.0f, 0.0f, "Detune Drift: Stereo movement", " cents", 0.0f, 50.0f, 0.0f);
    configParam(DETUNE_L_PARAM, -1.0f, 1.0f, 0.0f, "Left Channel Detune: ±50 cents", " cents", 0.0f, 50.0f);
    configParam(DETUNE_R_PARAM, -1.0f, 1.0f, 0.0f, "Right Channel Detune: ±50 cents", " cents", 0.0f, 50.0f);
    configParam(QUANTIZE_PARAM, 0.0f, 1.0f, 0.0f, "Quantize to Scale");
    configParam(MORPH_PARAM, 0.0f, 1.0f, 0.0f, "Morph: Blend between pitch algorithms");
    configParam(PITCH_MODE_PARAM, 0.0f, 4.0f, 1.0f, "Pitch Algorithm: Lo-Fi/H910/Varispeed/Hybrid/Spectral");
    configParam(CHARACTER_PARAM, 0.0f, 1.0f, 0.5f, "Character: Vintage pitch shifter modeling", "%", 0.0f, 100.0f);

    // Configure scale and tuning parameters (2)
    configParam(SCALE_SELECT_PARAM, 0.0f, 10.0f, 0.0f, "Scale Select (0-10)");
    configParam(MTS_ENABLE_PARAM, 0.0f, 1.0f, 0.0f, "MTS-ESP Enable");

    // Configure tape delay parameters (14+3)
    configParam(TAPE_MODE_PARAM, 0.0f, 1.0f, 0.0f, "Tape Mode Enable");
    configParam(WOW_RATE_PARAM, 0.1f, 5.0f, 0.3f, "Wow Rate", " Hz");
    configParam(WOW_DEPTH_PARAM, 0.0f, 1.0f, 0.2f, "Wow Depth", "%", 0.0f, 100.0f);
    configParam(FLUTTER_RATE_PARAM, 1.0f, 10.0f, 2.7f, "Flutter Rate", " Hz");
    configParam(FLUTTER_DEPTH_PARAM, 0.0f, 1.0f, 0.1f, "Flutter Depth", "%", 0.0f, 100.0f);
    configParam(WOW_WAVEFORM_PARAM, 0.0f, 2.0f, 0.0f, "Wow Waveform: Sine/Triangle/Random");
    configParam(FLUTTER_WAVEFORM_PARAM, 0.0f, 2.0f, 0.0f, "Flutter Waveform: Sine/Triangle/Random");
    configParam(SATURATION_PARAM, 0.0f, 1.0f, 0.3f, "Tape Saturation", "%", 0.0f, 100.0f);
    configParam(HEAD_BUMP_FREQ_PARAM, 60.0f, 120.0f, 90.0f, "Head Bump Frequency", " Hz");
    configParam(HEAD_BUMP_GAIN_PARAM, 0.0f, 3.0f, 1.2f, "Head Bump Gain", "x");
    configParam(ROLLOFF_FREQ_PARAM, 3000.0f, 15000.0f, 10000.0f, "High Frequency Rolloff", " Hz");
    configParam(ROLLOFF_RESONANCE_PARAM, 0.5f, 2.0f, 0.7f, "Rolloff Resonance");
    configParam(TAPE_NOISE_PARAM, 0.0f, 1.0f, 0.0f, "Tape Noise Enable");
    configParam(NOISE_AMOUNT_PARAM, 0.0f, 1.0f, 0.1f, "Tape Noise Amount", "%", 0.0f, 8.0f);
    configParam(HEAD_SELECT_PARAM, 0.0f, 3.0f, 0.0f, "Head Configuration (1-4)");
    configParam(AGING_PARAM, 0.0f, 1.0f, 0.0f, "Tape Aging", "%", 0.0f, 100.0f);
    configParam(INSTABILITY_PARAM, 0.0f, 1.0f, 0.0f, "Tape Instability", "%", 0.0f, 100.0f);

    // === SHIMMER + FEEDBACK PARAMETERS ===

    // Shimmer / Feedback architecture
    configParam(PITCH_PLACEMENT_PARAM, 0.0f, 1.0f, 0.0f, "Pitch Placement: Pre-loop/In-loop (Shimmer)");
    configParam(SHIMMER_PITCH_PARAM, -24.0f, 24.0f, 12.0f, "Shimmer Pitch Step", " semitones");
    configParam(SHIMMER_MIX_PARAM, 0.0f, 1.0f, 1.0f, "Shimmer Mix", "%", 0.0f, 100.0f);
    configParam(PITCH_DIRECTION_PARAM, 0.0f, 2.0f, 0.0f, "Pitch Direction: Up/Down/Alternate");
    configParam(SHIMMER_QUANTIZE_PARAM, 0.0f, 1.0f, 0.0f, "Shimmer Quantize to Scale");

    // Feedback path processing
    configParam(FB_LP_FREQ_PARAM, 200.0f, 20000.0f, 8000.0f, "Feedback LP Frequency", " Hz");
    configParam(FB_HP_FREQ_PARAM, 20.0f, 2000.0f, 80.0f, "Feedback HP Frequency", " Hz");
    configParam(FB_TILT_PARAM, -1.0f, 1.0f, 0.0f, "Feedback Tilt EQ");
    configParam(FB_DRIVE_PARAM, 0.0f, 1.0f, 0.0f, "Feedback Drive", "%", 0.0f, 100.0f);
    configParam(FB_COMP_PARAM, 0.0f, 1.0f, 0.0f, "Feedback Compression", "%", 0.0f, 100.0f);

    // Ducking
    configParam(DUCK_AMOUNT_PARAM, 0.0f, 1.0f, 0.0f, "Duck Amount", "%", 0.0f, 100.0f);
    configParam(DUCK_RELEASE_PARAM, 0.01f, 1.0f, 0.1f, "Duck Release", " s");

    // Pitch drift
    configParam(PITCH_DRIFT_PARAM, 0.0f, 50.0f, 0.0f, "Pitch Drift", " cents");
    configParam(DRIFT_RATE_PARAM, 0.1f, 5.0f, 0.5f, "Drift Rate", " Hz");

    // Freeze
    configParam(FREEZE_PARAM, 0.0f, 1.0f, 0.0f, "Freeze / Infinite Hold");

    // Frequency shifting (Ghost mode)
    configParam(FREQ_SHIFT_PARAM, -50.0f, 50.0f, 0.0f, "Frequency Shift", " Hz");
    configParam(FREQ_SHIFT_MODE_PARAM, 0.0f, 1.0f, 0.0f, "Frequency Shift Mode");

    // Independent L/R pitch
    configParam(PITCH_L_PARAM, -12.0f, 12.0f, 0.0f, "Left Pitch Offset", " semitones");
    configParam(PITCH_R_PARAM, -12.0f, 12.0f, 0.0f, "Right Pitch Offset", " semitones");

    // Pitch envelope follower
    configParam(PITCH_ENV_AMOUNT_PARAM, -1.0f, 1.0f, 0.0f, "Pitch Envelope Amount");
    configParam(PITCH_ENV_SPEED_PARAM, 0.0f, 1.0f, 0.5f, "Pitch Envelope Speed");

    // Reverse-granular (Crystallizer)
    configParam(REVERSE_GRAIN_PARAM, 0.0f, 1.0f, 0.0f, "Reverse Grain Mix", "%", 0.0f, 100.0f);
    configParam(REVERSE_GRAIN_SIZE_PARAM, 20.0f, 500.0f, 100.0f, "Reverse Grain Size", " ms");

    // Bloom mode
    configParam(BLOOM_AMOUNT_PARAM, 0.0f, 1.0f, 0.0f, "Bloom Amount", "%", 0.0f, 100.0f);
    configParam(BLOOM_RATE_PARAM, 0.1f, 5.0f, 0.5f, "Bloom Rate", " Hz");

    // Time stretch (spectral mode only)
    configParam(TIME_STRETCH_PARAM, 0.5f, 2.0f, 1.0f, "Time Stretch", "x");

    // Configure all inputs
    configInput(LEFT_INPUT, "Left Audio");
    configInput(RIGHT_INPUT, "Right Audio");
    configInput(TIME_L_CV_INPUT, "Left Time CV");
    configInput(TIME_R_CV_INPUT, "Right Time CV");
    configInput(FEEDBACK_L_CV_INPUT, "Left Feedback CV");
    configInput(FEEDBACK_R_CV_INPUT, "Right Feedback CV");
    configInput(MIX_L_CV_INPUT, "Left Mix CV");
    configInput(MIX_R_CV_INPUT, "Right Mix CV");
    configInput(TIME_MOD_INPUT, "Global Time Modulation CV");
    configInput(FEEDBACK_MOD_INPUT, "Global Feedback Modulation CV");
    configInput(TAP_L_TRIGGER_INPUT, "Left Tap Tempo");
    configInput(TAP_R_TRIGGER_INPUT, "Right Tap Tempo");
    configInput(PITCH_CV_INPUT, "Pitch CV");
    configInput(DETUNE_DRIFT_CV_INPUT, "Detune Drift CV");
    configInput(DETUNE_L_CV_INPUT, "Left Detune CV");
    configInput(DETUNE_R_CV_INPUT, "Right Detune CV");
    configInput(MORPH_CV_INPUT, "Morph CV");
    configInput(INPUT_GAIN_CV_INPUT, "Input Gain CV");
    configInput(OUTPUT_GAIN_CV_INPUT, "Output Gain CV");
    configInput(CHARACTER_CV_INPUT, "Character CV");
    configInput(WOW_RATE_CV_INPUT, "Wow Rate CV");
    configInput(WOW_DEPTH_CV_INPUT, "Wow Depth CV");
    configInput(FLUTTER_RATE_CV_INPUT, "Flutter Rate CV");
    configInput(FLUTTER_DEPTH_CV_INPUT, "Flutter Depth CV");
    configInput(SATURATION_CV_INPUT, "Saturation CV");
    configInput(TAPE_NOISE_CV_INPUT, "Tape Noise CV");
    configInput(NOISE_AMOUNT_CV_INPUT, "Noise Amount CV");
    configInput(AGING_CV_INPUT, "Aging CV");
    configInput(INSTABILITY_CV_INPUT, "Instability CV");
    configInput(HEAD_SELECT_CV_INPUT, "Head Select CV");
    // Shimmer + feedback inputs
    configInput(SHIMMER_PITCH_CV_INPUT, "Shimmer Pitch Step CV");
    configInput(FB_LP_FREQ_CV_INPUT, "Feedback LP Frequency CV");
    configInput(FB_HP_FREQ_CV_INPUT, "Feedback HP Frequency CV");
    configInput(DUCK_AMOUNT_CV_INPUT, "Duck Amount CV");
    configInput(FREQ_SHIFT_CV_INPUT, "Frequency Shift CV");
    configInput(PITCH_L_CV_INPUT, "Left Pitch Offset CV");
    configInput(PITCH_R_CV_INPUT, "Right Pitch Offset CV");
    configInput(FREEZE_CV_INPUT, "Freeze Gate");
    configInput(PITCH_ENV_CV_INPUT, "Pitch Envelope Amount CV");
    configInput(REVERSE_GRAIN_CV_INPUT, "Reverse Grain Mix CV");
    configInput(BLOOM_CV_INPUT, "Bloom Amount CV");
    configInput(PITCH_VOCT_INPUT, "V/Oct Pitch");
    configInput(TIME_STRETCH_CV_INPUT, "Time Stretch CV");

    // Configure all outputs
    configOutput(LEFT_OUTPUT, "Left Audio");
    configOutput(RIGHT_OUTPUT, "Right Audio");
    configOutput(WET_LEFT_OUTPUT, "Wet Left Audio");
    configOutput(WET_RIGHT_OUTPUT, "Wet Right Audio");

    // Initialize audio processing components
    float sampleRate = APP->engine->getSampleRate();
    leftDelay.configure(sampleRate);
    rightDelay.configure(sampleRate);
    tapeProcessor.configure(sampleRate);
    feedbackProc.configure(sampleRate);

    scalaReader.setDefaultScale();
    leftTapTrigger.reset();
    rightTapTrigger.reset();
}

// ===== RESET AND CONFIGURATION =====
void CurveAndDragModule::onReset() {
    leftDelay.reset();
    rightDelay.reset();
    tapeProcessor.reset();
    feedbackProc.reset();

    leftTapTimer.reset();
    rightTapTimer.reset();
    leftTapTrigger.reset();
    rightTapTrigger.reset();

    leftLevelSmooth = 0.0f;
    rightLevelSmooth = 0.0f;
    lastRawPitch = 0.0f;
    lastQuantizedPitch = 0.0f;
    processCounter = 0;

    pitchCVModulation = 0.0f;
    outputGainModulation = 0.0f;
    timeCVGlobal = 0.0f;
    feedbackCVGlobal = 0.0f;

    smoothedBasePitch = 0.0f;
    smoothedDetuneL = 0.0f;
    smoothedDetuneR = 0.0f;
    smoothedDrift = 0.0f;

    leftBBDBuffer.fill(0.0f);
    rightBBDBuffer.fill(0.0f);
    bbdIndex = 0;
    leftBBDPhase = 0.0f;
    rightBBDPhase = 0.0f;

    leftH910Buffer.fill(0.0f);
    rightH910Buffer.fill(0.0f);
    h910Index = 0;
    leftGrainPhase = 0.0f;
    rightGrainPhase = 0.0f;

    leftVarBuffer.fill(0.0f);
    rightVarBuffer.fill(0.0f);
    leftVarReadPos = 0.0f;
    rightVarReadPos = 0.0f;
    varWritePos = 0;

    leftHybridBuffer.fill(0.0f);
    rightHybridBuffer.fill(0.0f);
    hybridIndex = 0;
    leftHybridPhase = 0.0f;
    rightHybridPhase = 0.0f;

    prevLeftDelayed = 0.0f;
    prevRightDelayed = 0.0f;
    leftCrossFilter = 0.0f;
    rightCrossFilter = 0.0f;

    bbdClockPhaseL = 0.0f;
    bbdClockPhaseR = 0.0f;
    bbdPrevSampleL = 0.0f;
    bbdPrevSampleR = 0.0f;

    h910DriftL = 0.0f;
    h910DriftR = 0.0f;
    h910DriftPhaseL = 0.0f;
    h910DriftPhaseR = 0.0f;

    antiAliasFilterL1.reset();
    antiAliasFilterR1.reset();
    antiAliasFilterL2.reset();
    antiAliasFilterR2.reset();
    lastAntiAliasRatio = 0.0f;
    bbdLpfL = 0.0f;
    bbdLpfR = 0.0f;

    dcBlockerL.reset();
    dcBlockerR.reset();
    fbDcBlockerL.reset();
    fbDcBlockerR.reset();

    lastLeftSubdiv = -1.0f;
    lastRightSubdiv = -1.0f;

    // BBD compander and phase-locking pitch shifter reset
    bbdCompanderCompress.reset();
    bbdCompanderExpand.reset();
    phaseLockShifterL.reset();
    phaseLockShifterR.reset();
    spectralShifterL.reset();
    spectralShifterR.reset();

    // Shimmer + effects state reset
    reverseGrainBufL.fill(0.0f);
    reverseGrainBufR.fill(0.0f);
    reverseGrainWritePos = 0;
    reverseGrainPhaseL = 0.0f;
    reverseGrainPhaseR = 0.0f;
    bloomPhaseL = 0.0f;
    bloomPhaseR = 0.0f;
    bloomDelayBufL.fill(0.0f);
    bloomDelayBufR.fill(0.0f);
    bloomWritePosL = 0;
    bloomWritePosR = 0;
    pitchEnvFollowerL.reset();
    pitchEnvFollowerR.reset();
    freezeActive = false;
    inputLevelL = 0.0f;
    inputLevelR = 0.0f;
    pitchPlacement = PITCH_PRE_LOOP;
    ditherSeed = 12345;
    scalaReader.setDefaultScale();
}

void CurveAndDragModule::onSampleRateChange() {
    float sampleRate = APP->engine->getSampleRate();
    leftDelay.configure(sampleRate);
    rightDelay.configure(sampleRate);
    tapeProcessor.configure(sampleRate);
    feedbackProc.configure(sampleRate);
    bbdCompanderCompress.configure(sampleRate);
    bbdCompanderExpand.configure(sampleRate);

    // Reset pitch shifter buffers (content was recorded at old sample rate)
    phaseLockShifterL.reset();
    phaseLockShifterR.reset();
    spectralShifterL.reset();
    spectralShifterR.reset();
    leftBBDBuffer.fill(0.0f);
    rightBBDBuffer.fill(0.0f);
    leftH910Buffer.fill(0.0f);
    rightH910Buffer.fill(0.0f);
    leftVarBuffer.fill(0.0f);
    rightVarBuffer.fill(0.0f);
}

// ===== CV INPUT PROCESSING =====
void CurveAndDragModule::processAllCVInputs() {
    // Process core delay CV inputs (6)
    if (inputs[TIME_L_CV_INPUT].isConnected()) {
        float cvValue = inputs[TIME_L_CV_INPUT].getVoltage() / 10.0f;
        float currentTime = params[TIME_L_PARAM].getValue();
        float newTime = clamp(currentTime + cvValue * 0.2f, 0.0f, 1.0f);
        leftDelay.setDelayTime(newTime * 2000.0f);
    }

    if (inputs[TIME_R_CV_INPUT].isConnected()) {
        float cvValue = inputs[TIME_R_CV_INPUT].getVoltage() / 10.0f;
        float currentTime = params[TIME_R_PARAM].getValue();
        float newTime = clamp(currentTime + cvValue * 0.2f, 0.0f, 1.0f);
        rightDelay.setDelayTime(newTime * 2000.0f);
    }

    if (inputs[FEEDBACK_L_CV_INPUT].isConnected()) {
        float cvValue = inputs[FEEDBACK_L_CV_INPUT].getVoltage() / 10.0f;
        float currentFeedback = params[FEEDBACK_L_PARAM].getValue();
        leftDelay.setFeedback(clamp(currentFeedback + cvValue * 0.1f, 0.0f, 1.1f));
    }

    if (inputs[FEEDBACK_R_CV_INPUT].isConnected()) {
        float cvValue = inputs[FEEDBACK_R_CV_INPUT].getVoltage() / 10.0f;
        float currentFeedback = params[FEEDBACK_R_PARAM].getValue();
        rightDelay.setFeedback(clamp(currentFeedback + cvValue * 0.1f, 0.0f, 1.1f));
    }

    if (inputs[MIX_L_CV_INPUT].isConnected()) {
        float cvValue = inputs[MIX_L_CV_INPUT].getVoltage() / 10.0f;
        float mixParam = clamp(params[MIX_L_PARAM].getValue() + cvValue, 0.0f, 1.0f);
        leftDelay.setDryWet(mixParam);
    }

    if (inputs[MIX_R_CV_INPUT].isConnected()) {
        float cvValue = inputs[MIX_R_CV_INPUT].getVoltage() / 10.0f;
        float mixParam = clamp(params[MIX_R_PARAM].getValue() + cvValue, 0.0f, 1.0f);
        rightDelay.setDryWet(mixParam);
    }

    // Global modulation CV
    timeCVGlobal = inputs[TIME_MOD_INPUT].isConnected() ? inputs[TIME_MOD_INPUT].getVoltage() / 10.0f : 0.0f;
    feedbackCVGlobal = inputs[FEEDBACK_MOD_INPUT].isConnected() ? inputs[FEEDBACK_MOD_INPUT].getVoltage() / 10.0f : 0.0f;

    // Pitch CV
    if (inputs[PITCH_CV_INPUT].isConnected()) {
        pitchCVModulation = inputs[PITCH_CV_INPUT].getVoltage() * 20.0f;
    } else {
        pitchCVModulation = 0.0f;
    }

    // V/Oct input for MIDI-driven pitch intervals
    if (inputs[PITCH_VOCT_INPUT].isConnected()) {
        float voct = inputs[PITCH_VOCT_INPUT].getVoltage();
        pitchCVModulation += voct * 1200.0f; // 1V = 1 octave = 1200 cents
    }
}

// ===== DISPLAY UPDATES =====
void CurveAndDragModule::updateParameterDisplays() {
    processScaleSelection();

    if (params[SUBDIV_L_PARAM].getValue() < 0.0f || params[SUBDIV_L_PARAM].getValue() > 5.0f) {
        params[SUBDIV_L_PARAM].setValue(clamp(params[SUBDIV_L_PARAM].getValue(), 0.0f, 5.0f));
    }
    if (params[SUBDIV_R_PARAM].getValue() < 0.0f || params[SUBDIV_R_PARAM].getValue() > 5.0f) {
        params[SUBDIV_R_PARAM].setValue(clamp(params[SUBDIV_R_PARAM].getValue(), 0.0f, 5.0f));
    }

    float currentLeftSubdivVal = params[SUBDIV_L_PARAM].getValue();
    float currentRightSubdivVal = params[SUBDIV_R_PARAM].getValue();

    if (currentLeftSubdivVal != lastLeftSubdiv && params[SYNC_L_PARAM].getValue() > 0.5f) {
        processTempo();
        lastLeftSubdiv = currentLeftSubdivVal;
    }

    if (currentRightSubdivVal != lastRightSubdiv && params[SYNC_R_PARAM].getValue() > 0.5f) {
        processTempo();
        lastRightSubdiv = currentRightSubdivVal;
    }
}

// ===== TEMPO SYNC =====
void CurveAndDragModule::processTempo() {
    if (params[SYNC_L_PARAM].getValue() > 0.5f) {
        int subdivIndex = static_cast<int>(params[SUBDIV_L_PARAM].getValue());
        SubdivisionType subdivType = static_cast<SubdivisionType>(clamp(subdivIndex, 0, 5));
        float subdivMultiplier = getSubdivisionMultiplier(subdivType);
        float beatTimeMs = (60.0f / detectedBPM) * 1000.0f;
        float syncedDelayTime = beatTimeMs * subdivMultiplier;
        leftDelay.setDelayTime(clamp(syncedDelayTime, 1.0f, 2000.0f));
    }

    if (params[SYNC_R_PARAM].getValue() > 0.5f) {
        int subdivIndex = static_cast<int>(params[SUBDIV_R_PARAM].getValue());
        SubdivisionType subdivType = static_cast<SubdivisionType>(clamp(subdivIndex, 0, 5));
        float subdivMultiplier = getSubdivisionMultiplier(subdivType);
        float beatTimeMs = (60.0f / detectedBPM) * 1000.0f;
        float syncedDelayTime = beatTimeMs * subdivMultiplier;
        rightDelay.setDelayTime(clamp(syncedDelayTime, 1.0f, 2000.0f));
    }
}

// ===== TAPE MODE =====
void CurveAndDragModule::processTapeMode() {
    if (params[TAPE_MODE_PARAM].getValue() < 0.5f) {
        tapeProcessor.setTapeMode(false);
        return;
    }

    tapeProcessor.setTapeMode(true);

    float wowRate = getClampedParam(WOW_RATE_PARAM, WOW_RATE_CV_INPUT, 0.1f, 5.0f);
    float wowDepth = getClampedParam(WOW_DEPTH_PARAM, WOW_DEPTH_CV_INPUT, 0.0f, 1.0f);
    float flutterRate = getClampedParam(FLUTTER_RATE_PARAM, FLUTTER_RATE_CV_INPUT, 1.0f, 15.0f);
    float flutterDepth = getClampedParam(FLUTTER_DEPTH_PARAM, FLUTTER_DEPTH_CV_INPUT, 0.0f, 1.0f);

    int wowWaveformInt = static_cast<int>(params[WOW_WAVEFORM_PARAM].getValue());
    int flutterWaveformInt = static_cast<int>(params[FLUTTER_WAVEFORM_PARAM].getValue());
    WowFlutterWaveform wowWaveform = static_cast<WowFlutterWaveform>(clamp(wowWaveformInt, 0, 2));
    WowFlutterWaveform flutterWaveform = static_cast<WowFlutterWaveform>(clamp(flutterWaveformInt, 0, 2));

    tapeProcessor.setWowFlutter(wowRate, wowDepth, flutterRate, flutterDepth, wowWaveform, flutterWaveform);

    float saturation = getClampedParam(SATURATION_PARAM, SATURATION_CV_INPUT, 0.0f, 1.0f);
    tapeProcessor.setSaturation(saturation);

    float bumpFreq = getClampedParam(HEAD_BUMP_FREQ_PARAM, -1, 60.0f, 250.0f, 90.0f);
    float bumpGain = getClampedParam(HEAD_BUMP_GAIN_PARAM, -1, 0.5f, 3.0f, 1.2f);
    tapeProcessor.setHeadBump(bumpFreq, bumpGain, 1.2f);

    float rolloffFreq = getClampedParam(ROLLOFF_FREQ_PARAM, -1, 5000.0f, 15000.0f, 10000.0f);
    float rolloffRes = getClampedParam(ROLLOFF_RESONANCE_PARAM, -1, 0.1f, 2.0f, 0.7f);
    tapeProcessor.setRolloff(rolloffFreq, rolloffRes);

    float agingAmount = getClampedParam(AGING_PARAM, AGING_CV_INPUT, 0.0f, 1.0f);
    float instability = getClampedParam(INSTABILITY_PARAM, INSTABILITY_CV_INPUT, 0.0f, 1.0f);
    tapeProcessor.setAging(agingAmount);
    tapeProcessor.setInstability(instability);

    int headConfig = static_cast<int>(getClampedParam(HEAD_SELECT_PARAM, HEAD_SELECT_CV_INPUT, 0.0f, 3.0f));
    tapeProcessor.setHeadConfiguration(headConfig);

    // Configure per-head DSP from stored arrays
    for (int h = 0; h < 4; h++) {
        SVFilter::Mode fMode = static_cast<SVFilter::Mode>(headFilterMode[h]);
        for (int ch = 0; ch < 2; ch++) {
            tapeProcessor.setHeadDSP(ch, h, headPitchSemitones[h], 0.0f,
                                     fMode, headFilterCutoff[h], 0.5f,
                                     headLevel[h], headPan[h]);
        }
    }

    bool noiseEnabled = params[TAPE_NOISE_PARAM].getValue() > 0.5f;
    float noiseAmount = getClampedParam(NOISE_AMOUNT_PARAM, NOISE_AMOUNT_CV_INPUT, 0.0f, 1.0f);
    tapeProcessor.setNoiseParameters(noiseEnabled, noiseAmount);
}

// =====================================================================
// ===== v1.0 MAIN PROCESS FUNCTION — SHIMMER/CASCADE ARCHITECTURE =====
// =====================================================================
void CurveAndDragModule::process(const ProcessArgs& args) {
    // Denormal protection
#ifdef __SSE__
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
#endif
#ifdef __SSE3__
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
#endif

    processCounter++;

    if (processCounter % DISPLAY_UPDATE_RATE == 0) {
        updateParameterDisplays();
    }

    if (processCounter % MTS_POLL_RATE == 0) {
        mtsClient.pollForMtsConnection();
    }

    processAllCVInputs();
    processDelayParameters(args.sampleRate);
    processPitchParameters();
    processTapeMode();
    processTempo();
    processTapTempo(args.sampleRate);

    // === Determine pitch placement mode ===
    pitchPlacement = params[PITCH_PLACEMENT_PARAM].getValue() > 0.5f ? PITCH_IN_LOOP : PITCH_PRE_LOOP;

    // === Update feedback processor parameters ===
    feedbackProc.configure(args.sampleRate);
    feedbackProc.lpFreq = getClampedParam(FB_LP_FREQ_PARAM, FB_LP_FREQ_CV_INPUT, 200.0f, 20000.0f);
    feedbackProc.hpFreq = getClampedParam(FB_HP_FREQ_PARAM, FB_HP_FREQ_CV_INPUT, 20.0f, 2000.0f);
    feedbackProc.tiltAmount = params[FB_TILT_PARAM].getValue();
    feedbackProc.driveAmount = params[FB_DRIVE_PARAM].getValue();
    feedbackProc.compAmount = params[FB_COMP_PARAM].getValue();
    feedbackProc.duckAmount = getClampedParam(DUCK_AMOUNT_PARAM, DUCK_AMOUNT_CV_INPUT, 0.0f, 1.0f);
    feedbackProc.duckRelease = params[DUCK_RELEASE_PARAM].getValue();
    feedbackProc.driftAmount = params[PITCH_DRIFT_PARAM].getValue();
    feedbackProc.driftRate = params[DRIFT_RATE_PARAM].getValue();

    // Freeze handling: gate or toggle
    bool freezeGate = params[FREEZE_PARAM].getValue() > 0.5f;
    if (inputs[FREEZE_CV_INPUT].isConnected()) {
        freezeGate = freezeGate || (inputs[FREEZE_CV_INPUT].getVoltage() > 1.0f);
    }
    feedbackProc.freezeEnabled = freezeGate;
    freezeActive = freezeGate;

    // Frequency shift (Ghost mode)
    feedbackProc.freqShiftMode = params[FREQ_SHIFT_MODE_PARAM].getValue() > 0.5f;
    feedbackProc.freqShiftHz = getClampedParam(FREQ_SHIFT_PARAM, FREQ_SHIFT_CV_INPUT, -50.0f, 50.0f);

    // === Get input signals ===
    float leftInput = inputs[LEFT_INPUT].getVoltage() * 0.1f;
    float rightInput = inputs[RIGHT_INPUT].isConnected() ?
                      inputs[RIGHT_INPUT].getVoltage() * 0.1f : leftInput;

    // Apply input gain
    float inputGain = getClampedParam(INPUT_GAIN_PARAM, INPUT_GAIN_CV_INPUT, 0.0f, 2.0f);
    leftInput *= inputGain;
    rightInput *= inputGain;

    // Effective sample rate for DSP calculations
    float effectiveSampleRate = args.sampleRate;

    // Track input level for ducking (sample-rate independent)
    float levelSmooth = 1.0f - std::exp(-1.0f / (effectiveSampleRate * 0.003f));
    inputLevelL += (std::abs(leftInput) - inputLevelL) * levelSmooth;
    inputLevelR += (std::abs(rightInput) - inputLevelR) * levelSmooth;

    // === Pitch envelope follower ===
    float pitchEnvAmount = getClampedParam(PITCH_ENV_AMOUNT_PARAM, PITCH_ENV_CV_INPUT, -1.0f, 1.0f);
    float pitchEnvSpeed = params[PITCH_ENV_SPEED_PARAM].getValue();
    float envAttack = 1.0f - std::exp(-1.0f / (effectiveSampleRate * (0.001f + (1.0f - pitchEnvSpeed) * 0.1f)));
    float envRelease = 1.0f - std::exp(-1.0f / (effectiveSampleRate * (0.01f + (1.0f - pitchEnvSpeed) * 0.5f)));
    float envL = pitchEnvFollowerL.process(leftInput, envAttack, envRelease);
    float envR = pitchEnvFollowerR.process(rightInput, envAttack, envRelease);
    float envPitchModL = envL * pitchEnvAmount * 1200.0f; // Up to ±1 octave from envelope
    float envPitchModR = envR * pitchEnvAmount * 1200.0f;

    // === Get independent L/R pitch offsets ===
    float pitchOffsetL = getClampedParam(PITCH_L_PARAM, PITCH_L_CV_INPUT, -12.0f, 12.0f) * 100.0f; // semitones to cents
    float pitchOffsetR = getClampedParam(PITCH_R_PARAM, PITCH_R_CV_INPUT, -12.0f, 12.0f) * 100.0f;

    // === Compute total pre-loop pitch (classic mode pitch values) ===
    float leftPrePitch = lastDetuneL + pitchOffsetL + envPitchModL;
    float rightPrePitch = lastDetuneR + pitchOffsetR + envPitchModR;

    // === SIGNAL FLOW DEPENDS ON PITCH PLACEMENT ===

    float leftProcessed = leftInput;
    float rightProcessed = rightInput;
    float leftDelayed, rightDelayed;

    if (pitchPlacement == PITCH_PRE_LOOP) {
        // ===== CLASSIC MODE: Input → Pitch → Delay (internal feedback) → Output =====

        // Apply pitch shifting before delay
        if (std::abs(leftPrePitch) > 1.0f || std::abs(rightPrePitch) > 1.0f) {
            applyPitchShift(leftInput, rightInput, leftProcessed, rightProcessed,
                           leftPrePitch, rightPrePitch, effectiveSampleRate);
        }

        // DC blocking after pitch/saturation (2nd-order)
        leftProcessed = dcBlockerL.process(leftProcessed);
        rightProcessed = dcBlockerR.process(rightProcessed);

        // Process through delay lines (internal feedback)
        // process() handles feedback and write internally; use lastReadValue for raw wet signal
        leftDelay.process(leftProcessed);
        rightDelay.process(rightProcessed);
        leftDelayed = leftDelay.getLastReadValue();
        rightDelayed = rightDelay.getLastReadValue();

        // Reverse-granular on delay output (pre-loop mode)
        float reverseGrainMix = getClampedParam(REVERSE_GRAIN_PARAM, REVERSE_GRAIN_CV_INPUT, 0.0f, 1.0f);
        if (reverseGrainMix > 0.001f) {
            float grainSize = params[REVERSE_GRAIN_SIZE_PARAM].getValue();
            float revL = processReverseGrain(leftDelayed, 0, grainSize, effectiveSampleRate);
            float revR = processReverseGrain(rightDelayed, 1, grainSize, effectiveSampleRate);
            leftDelayed = leftDelayed * (1.0f - reverseGrainMix) + revL * reverseGrainMix;
            rightDelayed = rightDelayed * (1.0f - reverseGrainMix) + revR * reverseGrainMix;
        }

        // Bloom/chorus on delay output (pre-loop mode)
        float bloomAmount = getClampedParam(BLOOM_AMOUNT_PARAM, BLOOM_CV_INPUT, 0.0f, 1.0f);
        if (bloomAmount > 0.001f) {
            float bloomRate = params[BLOOM_RATE_PARAM].getValue();
            leftDelayed = processBloom(leftDelayed, 0, bloomAmount, bloomRate, effectiveSampleRate);
            rightDelayed = processBloom(rightDelayed, 1, bloomAmount, bloomRate, effectiveSampleRate);
        }

    } else {
        // ===== SHIMMER MODE: Input → Delay → [Pitch + Filter + Saturation] → Feedback =====
        // The pitch shift happens INSIDE the feedback loop, creating compounding shimmer

        // Get shimmer parameters
        float shimmerPitchSemitones = getClampedParam(SHIMMER_PITCH_PARAM, SHIMMER_PITCH_CV_INPUT, -24.0f, 24.0f);
        float shimmerMix = params[SHIMMER_MIX_PARAM].getValue();
        int pitchDir = clamp(static_cast<int>(params[PITCH_DIRECTION_PARAM].getValue()), 0, 2);
        bool shimmerQuantize = params[SHIMMER_QUANTIZE_PARAM].getValue() > 0.5f;

        // Compute shimmer pitch in cents
        float shimmerPitchCents = shimmerPitchSemitones * 100.0f;

        // Apply direction
        if (pitchDir == PITCH_DIR_DOWN) {
            shimmerPitchCents = -shimmerPitchCents;
        } else if (pitchDir == PITCH_DIR_ALTERNATE) {
            // Alternate based on a slow LFO (toggles every ~0.5s)
            // Use double precision fmod to avoid precision loss after extended runtime
            double altPhaseD = std::fmod(static_cast<double>(processCounter), static_cast<double>(effectiveSampleRate) * 0.5);
            float altPhase = std::sin(static_cast<float>(altPhaseD / (static_cast<double>(effectiveSampleRate) * 0.5) * 2.0 * M_PI));
            shimmerPitchCents *= (altPhase > 0.0f) ? 1.0f : -1.0f;
        }

        // Add pitch drift
        float driftL = feedbackProc.getPitchDrift(0);
        float driftR = feedbackProc.getPitchDrift(1);

        // Quantize shimmer pitch to scale if enabled
        if (shimmerQuantize) {
            shimmerPitchCents = quantizePitchToScale(shimmerPitchCents);
        }

        // Total in-loop pitch = shimmer step + drift + envelope mod + independent L/R offset
        float inLoopPitchL = shimmerPitchCents + driftL + envPitchModL + pitchOffsetL;
        float inLoopPitchR = shimmerPitchCents + driftR + envPitchModR + pitchOffsetR;

        // Also apply the pre-loop pitch (base pitch knob) to the input before delay
        if (std::abs(lastDetuneL) > 1.0f || std::abs(lastDetuneR) > 1.0f) {
            applyPitchShift(leftInput, rightInput, leftProcessed, rightProcessed,
                           lastDetuneL, lastDetuneR, effectiveSampleRate);
        }

        // DC blocking after pitch (2nd-order)
        leftProcessed = dcBlockerL.process(leftProcessed);
        rightProcessed = dcBlockerR.process(rightProcessed);

        // Read delayed signals (WITHOUT writing feedback yet)
        float leftDelayedRaw = leftDelay.readDelayed();
        float rightDelayedRaw = rightDelay.readDelayed();

        // === FEEDBACK PROCESSING CHAIN ===
        // 1. Pitch shift the delayed signal (shimmer!)
        float leftFBPitched = leftDelayedRaw;
        float rightFBPitched = rightDelayedRaw;

        if (!freezeActive && (std::abs(inLoopPitchL) > 1.0f || std::abs(inLoopPitchR) > 1.0f)) {
            applyPitchShift(leftDelayedRaw, rightDelayedRaw,
                           leftFBPitched, rightFBPitched,
                           inLoopPitchL, inLoopPitchR, effectiveSampleRate);

            // Mix pitched and unpitched in feedback
            leftFBPitched = leftDelayedRaw * (1.0f - shimmerMix) + leftFBPitched * shimmerMix;
            rightFBPitched = rightDelayedRaw * (1.0f - shimmerMix) + rightFBPitched * shimmerMix;
        }

        // 2. Reverse-granular processing (Crystallizer-style)
        float reverseGrainMix = getClampedParam(REVERSE_GRAIN_PARAM, REVERSE_GRAIN_CV_INPUT, 0.0f, 1.0f);
        if (reverseGrainMix > 0.001f) {
            float grainSize = params[REVERSE_GRAIN_SIZE_PARAM].getValue();
            float revL = processReverseGrain(leftFBPitched, 0, grainSize, effectiveSampleRate);
            float revR = processReverseGrain(rightFBPitched, 1, grainSize, effectiveSampleRate);
            leftFBPitched = leftFBPitched * (1.0f - reverseGrainMix) + revL * reverseGrainMix;
            rightFBPitched = rightFBPitched * (1.0f - reverseGrainMix) + revR * reverseGrainMix;
        }

        // 3. Bloom/chorus in feedback (RichPitch-style)
        float bloomAmount = getClampedParam(BLOOM_AMOUNT_PARAM, BLOOM_CV_INPUT, 0.0f, 1.0f);
        if (bloomAmount > 0.001f) {
            float bloomRate = params[BLOOM_RATE_PARAM].getValue();
            leftFBPitched = processBloom(leftFBPitched, 0, bloomAmount, bloomRate, effectiveSampleRate);
            rightFBPitched = processBloom(rightFBPitched, 1, bloomAmount, bloomRate, effectiveSampleRate);
        }

        // 4. DC block feedback path to prevent DC accumulation from saturation/pitch
        leftFBPitched = fbDcBlockerL.process(leftFBPitched);
        rightFBPitched = fbDcBlockerR.process(rightFBPitched);

        // 5. Feedback path processing (filters, saturation, compression, ducking, freq shift)
        leftFBPitched = feedbackProc.process(leftFBPitched, 0, inputLevelL);
        rightFBPitched = feedbackProc.process(rightFBPitched, 1, inputLevelR);

        // 5. Apply feedback amount and write back to delay line
        float feedbackL = leftDelay.getFeedback();
        float feedbackR = rightDelay.getFeedback();

        // Freeze: hold feedback at unity, don't add new input
        if (freezeActive) {
            feedbackL = 1.0f;
            feedbackR = 1.0f;
            float freezeCF = feedbackProc.getFreezeCrossfade(0);
            float inputMixL = leftProcessed * (1.0f - freezeCF);
            float inputMixR = rightProcessed * (1.0f - freezeCF);
            leftDelay.writeWithFeedback(inputMixL, leftFBPitched * feedbackL);
            rightDelay.writeWithFeedback(inputMixR, rightFBPitched * feedbackR);
        } else {
            leftDelay.writeWithFeedback(leftProcessed, leftFBPitched * feedbackL);
            rightDelay.writeWithFeedback(rightProcessed, rightFBPitched * feedbackR);
        }

        // Use pitch-shifted signals for output so shimmer effect is audible
        leftDelayed = leftFBPitched;
        rightDelayed = rightFBPitched;
    }

    // === Cross-Feedback (BEFORE Tape Processing) ===
    if (params[CROSS_FEEDBACK_PARAM].getValue() > 0.01f) {
        float crossAmount = clamp(params[CROSS_FEEDBACK_PARAM].getValue() * 0.3f, 0.0f, 0.3f);

        float crossFilterCoeff = 1.0f - std::exp(-1.0f / (effectiveSampleRate * 0.00003f)); // ~30us time constant
        leftCrossFilter += (prevRightDelayed - leftCrossFilter) * crossFilterCoeff;
        rightCrossFilter += (prevLeftDelayed - rightCrossFilter) * crossFilterCoeff;

        float leftCross = leftDelayed + leftCrossFilter * crossAmount;
        float rightCross = rightDelayed + rightCrossFilter * crossAmount;

        leftDelayed = std::tanh(leftCross * 0.7f) / 0.7f;
        rightDelayed = std::tanh(rightCross * 0.7f) / 0.7f;
    }

    // === Tape Processing (AFTER Cross-Feedback) ===
    bool tapeEnabled = params[TAPE_MODE_PARAM].getValue() > 0.5f;
    if (tapeEnabled) {
        int currentPitchMode = clamp(static_cast<int>(params[PITCH_MODE_PARAM].getValue()), 0, 4);
        float currentCharacter = params[CHARACTER_PARAM].getValue();
        bool hasPitchSaturation = (std::abs(lastDetuneL) > 1.0f || std::abs(lastDetuneR) > 1.0f);

        if (hasPitchSaturation && (currentPitchMode == 0 || currentCharacter > 0.3f)) {
            float origSat = params[SATURATION_PARAM].getValue();
            float reducedSat = origSat * (1.0f - currentCharacter * 0.4f);
            if (currentPitchMode == 0) reducedSat *= 0.7f;
            tapeProcessor.setSaturation(reducedSat);
        }

        leftDelayed = tapeProcessor.process(leftDelayed, 0);
        rightDelayed = tapeProcessor.process(rightDelayed, 1);
    }

    prevLeftDelayed = leftDelayed;
    prevRightDelayed = rightDelayed;

    // === Output Mixing and Gain ===
    float outputGain = getClampedParam(OUTPUT_GAIN_PARAM, OUTPUT_GAIN_CV_INPUT, 0.0f, 2.0f);

    float leftMix = clamp(params[MIX_L_PARAM].getValue(), 0.0f, 1.0f);
    float rightMix = clamp(params[MIX_R_PARAM].getValue(), 0.0f, 1.0f);

    float leftOutput = (leftInput * (1.0f - leftMix) + leftDelayed * leftMix) * outputGain;
    float rightOutput = (rightInput * (1.0f - rightMix) + rightDelayed * rightMix) * outputGain;

    leftOutput = softClip(leftOutput);
    rightOutput = softClip(rightOutput);
    leftOutput = clamp(leftOutput, -5.0f, 5.0f);
    rightOutput = clamp(rightOutput, -5.0f, 5.0f);

    // TPDF dither at 24-bit level
    {
        auto tpdfDither = [this]() -> float {
            ditherSeed = ditherSeed * 1664525 + 1013904223;
            float r1 = (ditherSeed / 4294967296.0f) - 0.5f;
            ditherSeed = ditherSeed * 1664525 + 1013904223;
            float r2 = (ditherSeed / 4294967296.0f) - 0.5f;
            return (r1 + r2) * (1.0f / (1 << 23));
        };
        leftOutput += tpdfDither();
        rightOutput += tpdfDither();
    }

    outputs[LEFT_OUTPUT].setVoltage(leftOutput * 10.0f);
    outputs[RIGHT_OUTPUT].setVoltage(rightOutput * 10.0f);

    outputs[WET_LEFT_OUTPUT].setVoltage(leftDelayed * outputGain * 10.0f);
    outputs[WET_RIGHT_OUTPUT].setVoltage(rightDelayed * outputGain * 10.0f);

    // === Update lights ===
    if (processCounter % LEVEL_UPDATE_RATE == 0) {
        updateLevelMeters(leftInput, rightInput);
        updateStatusLights();
    }
}

// ===== PITCH SHIFTING ENGINE =====
// Extracted to be reusable for both pre-loop and in-loop processing
void CurveAndDragModule::applyPitchShift(float leftIn, float rightIn,
                                          float& leftOut, float& rightOut,
                                          float leftPitchCents, float rightPitchCents,
                                          float sr) {
    int pitchMode = clamp(static_cast<int>(params[PITCH_MODE_PARAM].getValue()), 0, 4);
    float character = getClampedParam(CHARACTER_PARAM, CHARACTER_CV_INPUT, 0.0f, 1.0f);
    float morph = getClampedParam(MORPH_PARAM, MORPH_CV_INPUT, 0.0f, 1.0f);

    float leftRatio = std::pow(2.0f, leftPitchCents / 1200.0f);
    float rightRatio = std::pow(2.0f, rightPitchCents / 1200.0f);

    float leftGainComp = 1.0f / std::sqrt(std::abs(leftRatio));
    float rightGainComp = 1.0f / std::sqrt(std::abs(rightRatio));
    leftGainComp = clamp(leftGainComp, 0.5f, 2.0f);
    rightGainComp = clamp(rightGainComp, 0.5f, 2.0f);
    leftRatio = clamp(leftRatio, 0.25f, 4.0f);
    rightRatio = clamp(rightRatio, 0.25f, 4.0f);

    // Anti-alias pre-filter for pitch-up (4th order / 24dB per octave via cascaded biquads)
    if (leftRatio > 1.01f || rightRatio > 1.01f) {
        float maxRatio = std::max(leftRatio, rightRatio);
        if (std::abs(maxRatio - lastAntiAliasRatio) > 0.01f) {
            float cutoffFreq = sr / (2.0f * maxRatio);
            cutoffFreq = clamp(cutoffFreq, 100.0f, sr * 0.49f);
            float normalizedFreq = cutoffFreq / sr;
            // Butterworth 4th-order: two cascaded 2nd-order sections with Q values
            // Q1 = 0.5412 (1/(2*cos(pi/8))), Q2 = 1.3066 (1/(2*cos(3*pi/8)))
            antiAliasFilterL1.setParameters(rack::dsp::BiquadFilter::LOWPASS, normalizedFreq, 0.5412f, 1.0f);
            antiAliasFilterR1.setParameters(rack::dsp::BiquadFilter::LOWPASS, normalizedFreq, 0.5412f, 1.0f);
            antiAliasFilterL2.setParameters(rack::dsp::BiquadFilter::LOWPASS, normalizedFreq, 1.3066f, 1.0f);
            antiAliasFilterR2.setParameters(rack::dsp::BiquadFilter::LOWPASS, normalizedFreq, 1.3066f, 1.0f);
            lastAntiAliasRatio = maxRatio;
        }
        leftIn = antiAliasFilterL2.process(antiAliasFilterL1.process(leftIn));
        rightIn = antiAliasFilterR2.process(antiAliasFilterR1.process(rightIn));
    }

    auto hermite = [](float p0, float p1, float p2, float p3, float t) -> float {
        float c0 = p1;
        float c1 = 0.5f * (p2 - p0);
        float c2 = p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3;
        float c3 = 0.5f * (p3 - p0) + 1.5f * (p1 - p2);
        return ((c3 * t + c2) * t + c1) * t + c0;
    };

    float dryL = leftIn;
    float dryR = rightIn;

    int nextMode = (pitchMode + 1) % 5;
    bool needBBD = (pitchMode == 0 || (morph > 0.001f && nextMode == 0));
    bool needH910 = (pitchMode == 1 || (morph > 0.001f && nextMode == 1));
    bool needVar = (pitchMode == 2 || (morph > 0.001f && nextMode == 2));
    bool needHyb = (pitchMode == 3 || (morph > 0.001f && nextMode == 3));
    bool needSpec = (pitchMode == 4 || (morph > 0.001f && nextMode == 4));

    // Write to all buffers
    leftBBDBuffer[bbdIndex] = dryL;
    rightBBDBuffer[bbdIndex] = dryR;
    leftH910Buffer[h910Index] = dryL;
    rightH910Buffer[h910Index] = dryR;
    leftVarBuffer[varWritePos] = dryL;
    rightVarBuffer[varWritePos] = dryR;
    leftHybridBuffer[hybridIndex] = dryL;
    rightHybridBuffer[hybridIndex] = dryR;

    // === Algorithm 0: Lo-Fi (vintage BBD with NE570/SA571 compander modeling) ===
    float bbdL = dryL, bbdR = dryR;
    if (needBBD) {
        // BBD bandwidth limiting: real BBDs have ~3kHz LP on input
        float bbdCutoff = 0.15f + character * 0.05f;
        bbdLpfL += (dryL - bbdLpfL) * bbdCutoff;
        bbdLpfR += (dryR - bbdLpfR) * bbdCutoff;

        // NE570-style compander: compress before writing to BBD
        // Character controls how much companding is applied (0=none, 1=full)
        bbdCompanderCompress.configure(sr);
        float compL = bbdCompanderCompress.compress(bbdLpfL, 0, character);
        float compR = bbdCompanderCompress.compress(bbdLpfR, 1, character);

        // Charge transfer loss: gentle LP per stage (incomplete charge transfer)
        compL = compL * 0.998f + bbdPrevSampleL * 0.002f;
        compR = compR * 0.998f + bbdPrevSampleR * 0.002f;
        bbdPrevSampleL = compL;
        bbdPrevSampleR = compR;

        leftBBDBuffer[bbdIndex] = compL;
        rightBBDBuffer[bbdIndex] = compR;

        leftBBDPhase += 1.0f / leftRatio;
        rightBBDPhase += 1.0f / rightRatio;

        float lPos = bbdIndex - leftBBDPhase;
        float rPos = bbdIndex - rightBBDPhase;
        int li = static_cast<int>(std::floor(lPos));
        int ri = static_cast<int>(std::floor(rPos));
        float lf = lPos - std::floor(lPos);
        float rf = rPos - std::floor(rPos);

        bbdL = hermite(leftBBDBuffer[((li - 1) + 8192) & 8191], leftBBDBuffer[(li + 8192) & 8191],
                      leftBBDBuffer[((li + 1) + 8192) & 8191], leftBBDBuffer[((li + 2) + 8192) & 8191], lf);
        bbdR = hermite(rightBBDBuffer[((ri - 1) + 8192) & 8191], rightBBDBuffer[(ri + 8192) & 8191],
                      rightBBDBuffer[((ri + 1) + 8192) & 8191], rightBBDBuffer[((ri + 2) + 8192) & 8191], rf);

        // NE570-style expander: expand after reading from BBD
        bbdCompanderExpand.configure(sr);
        bbdL = bbdCompanderExpand.expand(bbdL, 0, character);
        bbdR = bbdCompanderExpand.expand(bbdR, 1, character);
        bbdL *= leftGainComp;
        bbdR *= rightGainComp;

        float clockRateL = sr * leftRatio * 0.5f;
        float clockRateR = sr * rightRatio * 0.5f;
        bbdClockPhaseL += clockRateL / sr;
        bbdClockPhaseR += clockRateR / sr;
        if (bbdClockPhaseL > 1.0f) bbdClockPhaseL -= 1.0f;
        if (bbdClockPhaseR > 1.0f) bbdClockPhaseR -= 1.0f;
        float clockAmount = 0.002f * character;
        bbdL += clockAmount * std::sin(2.0f * M_PI * bbdClockPhaseL);
        bbdR += clockAmount * std::sin(2.0f * M_PI * bbdClockPhaseR);

        if (character > 0.5f) {
            float noiseLevel = (character - 0.5f) * 2.0f * 0.0001f;
            // Use accumulating LCG state for non-periodic noise
            ditherSeed = ditherSeed * 1103515245 + 12345;
            float whiteL = (ditherSeed / 4294967296.0f) * 2.0f - 1.0f;
            ditherSeed = ditherSeed * 1103515245 + 12345;
            float whiteR = (ditherSeed / 4294967296.0f) * 2.0f - 1.0f;
            bbdL += whiteL * noiseLevel;
            bbdR += whiteR * noiseLevel;
        }

        if (leftBBDPhase > 4096.0f) leftBBDPhase -= 4096.0f;
        if (rightBBDPhase > 4096.0f) rightBBDPhase -= 4096.0f;
        bbdIndex = (bbdIndex + 1) & 8191;
    }

    // === Algorithm 1: H910 ===
    float h910L = dryL, h910R = dryR;
    if (needH910) {
        h910DriftPhaseL += 0.0003f + 0.0001f * std::sin(processCounter * 0.00017f);
        h910DriftPhaseR += 0.00033f + 0.0001f * std::sin(processCounter * 0.00013f);
        if (h910DriftPhaseL > 1.0f) h910DriftPhaseL -= 1.0f;
        if (h910DriftPhaseR > 1.0f) h910DriftPhaseR -= 1.0f;
        float driftLv = 1.0f + 0.03f * std::sin(2.0f * M_PI * h910DriftPhaseL) * character;
        float driftRv = 1.0f + 0.03f * std::sin(2.0f * M_PI * h910DriftPhaseR) * character;

        int lGrain = clamp(static_cast<int>(512.0f * driftLv / std::abs(leftRatio)), 128, 2048);
        int rGrain = clamp(static_cast<int>(512.0f * driftRv / std::abs(rightRatio)), 128, 2048);

        float lTriangle = 1.0f - 2.0f * std::abs(leftGrainPhase / lGrain - 0.5f);
        float rTriangle = 1.0f - 2.0f * std::abs(rightGrainPhase / rGrain - 0.5f);

        float lPos1 = h910Index - leftGrainPhase * leftRatio;
        float lPos2 = h910Index - (leftGrainPhase + lGrain * 0.5f) * leftRatio;
        float rPos1 = h910Index - rightGrainPhase * rightRatio;
        float rPos2 = h910Index - (rightGrainPhase + rGrain * 0.5f) * rightRatio;

        auto readH910 = [&](std::array<float, 4096>& buf, float pos) -> float {
            int i = static_cast<int>(std::floor(pos));
            float f = pos - std::floor(pos);
            return hermite(buf[(i - 1) & 4095], buf[i & 4095],
                         buf[(i + 1) & 4095], buf[(i + 2) & 4095], f);
        };

        float lTap1 = readH910(leftH910Buffer, lPos1);
        float lTap2 = readH910(leftH910Buffer, lPos2);
        float rTap1 = readH910(rightH910Buffer, rPos1);
        float rTap2 = readH910(rightH910Buffer, rPos2);

        h910L = (lTap1 * lTriangle + lTap2 * (1.0f - lTriangle)) * leftGainComp;
        h910R = (rTap1 * rTriangle + rTap2 * (1.0f - rTriangle)) * rightGainComp;

        leftGrainPhase += 1.0f;
        rightGrainPhase += 1.0f;
        if (leftGrainPhase >= lGrain) leftGrainPhase = 0.0f;
        if (rightGrainPhase >= rGrain) rightGrainPhase = 0.0f;
        h910Index = (h910Index + 1) & 4095;
    }

    // === Algorithm 2: Varispeed (dual-head crossfading) ===
    float varL = dryL, varR = dryR;
    if (needVar) {
        // Two read heads offset by half buffer, crossfaded to avoid lapping artifacts
        leftVarReadPos += leftRatio;
        rightVarReadPos += rightRatio;
        if (leftVarReadPos >= 16384.0f) leftVarReadPos -= 16384.0f;
        if (rightVarReadPos >= 16384.0f) rightVarReadPos -= 16384.0f;

        auto readVar = [&](std::array<float, 16384>& buf, float readPos) -> float {
            int li = static_cast<int>(std::floor(readPos));
            float lf = readPos - std::floor(readPos);
            return hermite(buf[(li - 1) & 16383], buf[li & 16383],
                          buf[(li + 1) & 16383], buf[(li + 2) & 16383], lf);
        };

        // Head 1: primary read position
        float lHead1 = readVar(leftVarBuffer, leftVarReadPos);
        float rHead1 = readVar(rightVarBuffer, rightVarReadPos);

        // Head 2: offset by half buffer
        float lReadPos2 = std::fmod(leftVarReadPos + 8192.0f, 16384.0f);
        float rReadPos2 = std::fmod(rightVarReadPos + 8192.0f, 16384.0f);
        float lHead2 = readVar(leftVarBuffer, lReadPos2);
        float rHead2 = readVar(rightVarBuffer, rReadPos2);

        // Crossfade based on distance from write pointer to avoid reading stale data
        auto crossfadeWeight = [&](float readPos) -> float {
            float dist = std::fmod(static_cast<float>(varWritePos) - readPos + 16384.0f, 16384.0f);
            float normalized = dist / 16384.0f;
            // Fade out when read head approaches write head (within 10% of buffer)
            if (normalized < 0.1f) return normalized * 10.0f;
            if (normalized > 0.9f) return (1.0f - normalized) * 10.0f;
            return 1.0f;
        };

        float lw1 = crossfadeWeight(leftVarReadPos);
        float lw2 = crossfadeWeight(lReadPos2);
        float rw1 = crossfadeWeight(rightVarReadPos);
        float rw2 = crossfadeWeight(rReadPos2);

        float lwSum = lw1 + lw2;
        float rwSum = rw1 + rw2;
        if (lwSum > 0.001f) { lw1 /= lwSum; lw2 /= lwSum; } else { lw1 = 0.5f; lw2 = 0.5f; }
        if (rwSum > 0.001f) { rw1 /= rwSum; rw2 /= rwSum; } else { rw1 = 0.5f; rw2 = 0.5f; }

        varL = (lHead1 * lw1 + lHead2 * lw2) * leftGainComp;
        varR = (rHead1 * rw1 + rHead2 * rw2) * rightGainComp;

        varWritePos = (varWritePos + 1) & 16383;
    }

    // === Algorithm 3: Hybrid (with Laroche-Dolson identity phase locking) ===
    // Uses 4-grain overlapping phase-locked pitch shifting for highest quality.
    // Small shifts use long grains (minimal artifacts), large shifts use short grains.
    // Phase coherence is maintained between grains to eliminate "phasiness".
    float hybL = dryL, hybR = dryR;
    if (needHyb) {
        // Use the Laroche-Dolson phase-locking pitch shifter
        hybL = phaseLockShifterL.process(dryL, leftRatio, character) * leftGainComp;
        hybR = phaseLockShifterR.process(dryR, rightRatio, character) * rightGainComp;

        // Apply character-dependent saturation for large shifts
        if (character > 0.3f && (std::abs(leftPitchCents) > 200.0f || std::abs(rightPitchCents) > 200.0f)) {
            float satAmount = (character - 0.3f) * 1.4f; // 0-1 range
            hybL = hybL * (1.0f - satAmount * 0.3f) + std::tanh(hybL * (1.0f + satAmount)) * satAmount * 0.3f;
            hybR = hybR * (1.0f - satAmount * 0.3f) + std::tanh(hybR * (1.0f + satAmount)) * satAmount * 0.3f;
        }

        // Still advance old buffer indices for morph transitions
        hybridIndex = (hybridIndex + 1) & 8191;
    }

    // === Algorithm 4: Spectral (FFT phase vocoder with formant preservation) ===
    float specL = dryL, specR = dryR;
    if (needSpec) {
        float timeStretch = getClampedParam(TIME_STRETCH_PARAM, TIME_STRETCH_CV_INPUT, 0.5f, 2.0f);
        spectralShifterL.processTimeStretch(dryL, leftRatio, timeStretch, character, specL, sr);
        spectralShifterR.processTimeStretch(dryR, rightRatio, timeStretch, character, specR, sr);
        specL *= leftGainComp;
        specR *= rightGainComp;
    }

    // === MORPH ===
    float algoOutputs[5][2] = {{bbdL, bbdR}, {h910L, h910R}, {varL, varR}, {hybL, hybR}, {specL, specR}};

    if (morph < 0.001f) {
        leftOut = algoOutputs[pitchMode][0];
        rightOut = algoOutputs[pitchMode][1];
    } else {
        float blend = morph;
        leftOut = algoOutputs[pitchMode][0] * (1.0f - blend) + algoOutputs[nextMode][0] * blend;
        rightOut = algoOutputs[pitchMode][1] * (1.0f - blend) + algoOutputs[nextMode][1] * blend;
    }

    // Advance write indices for unused algorithms
    if (!needBBD) bbdIndex = (bbdIndex + 1) & 8191;
    if (!needH910) h910Index = (h910Index + 1) & 4095;
    if (!needVar) varWritePos = (varWritePos + 1) & 16383;
    if (!needHyb) hybridIndex = (hybridIndex + 1) & 8191;

    // Character/vintage modeling post-pitch
    if (character > 0.001f) {
        if (character < 0.33f) {
            float warmth = character * 3.0f;
            float sat = 1.0f + warmth * 0.3f;
            leftOut = leftOut * (1.0f - warmth * 0.3f) +
                (std::tanh(leftOut * sat) + 0.1f * leftOut * leftOut) * warmth * 0.3f;
            rightOut = rightOut * (1.0f - warmth * 0.3f) +
                (std::tanh(rightOut * sat) + 0.1f * rightOut * rightOut) * warmth * 0.3f;
        } else if (character < 0.66f) {
            float grit = (character - 0.33f) * 3.0f;
            float sat = 1.0f + grit * 0.6f;
            leftOut = std::tanh(leftOut * sat) / sat;
            rightOut = std::tanh(rightOut * sat) / sat;
        } else {
            float aggression = (character - 0.66f) * 3.0f;
            float drive = 1.0f + aggression * 1.2f;
            leftOut = std::sin(leftOut * drive * M_PI * 0.5f) / (drive * 0.5f);
            rightOut = std::sin(rightOut * drive * M_PI * 0.5f) / (drive * 0.5f);
        }
    }
}

// ===== REVERSE-GRANULAR PROCESSING (Crystallizer-style) =====
float CurveAndDragModule::processReverseGrain(float input, int channel, float grainSizeMs, float sampleRate) {
    int grainSizeSamples = clamp(static_cast<int>(grainSizeMs * sampleRate / 1000.0f), 64, 16384);

    auto& buf = (channel == 0) ? reverseGrainBufL : reverseGrainBufR;
    float& phase = (channel == 0) ? reverseGrainPhaseL : reverseGrainPhaseR;

    // Write input to circular buffer
    buf[reverseGrainWritePos] = input;
    if (channel == 1) {
        reverseGrainWritePos = (reverseGrainWritePos + 1) & 16383;
    }

    // Read in reverse with Hann window
    phase += 1.0f;
    if (phase >= grainSizeSamples) phase = 0.0f;

    float normalizedPhase = phase / grainSizeSamples;
    float window = 0.5f * (1.0f - std::cos(2.0f * M_PI * normalizedPhase));

    // Read backwards from the start of this grain
    int grainStart = (reverseGrainWritePos - grainSizeSamples + 16384) & 16383;
    int reverseReadPos = (grainStart + grainSizeSamples - static_cast<int>(phase)) & 16383;

    return buf[reverseReadPos] * window;
}

// ===== BLOOM/CHORUS IN FEEDBACK (RichPitch-style modulated delay) =====
float CurveAndDragModule::processBloom(float input, int channel, float amount, float rate, float sampleRate) {
    float& phase = (channel == 0) ? bloomPhaseL : bloomPhaseR;
    auto& buf = (channel == 0) ? bloomDelayBufL : bloomDelayBufR;
    int& writePos = (channel == 0) ? bloomWritePosL : bloomWritePosR;

    // Write input to circular buffer
    buf[writePos] = input;
    writePos = (writePos + 1) & (BLOOM_BUFFER_SIZE - 1);

    // Advance LFO — R channel uses slightly different rate for stereo spread
    float effectiveRate = (channel == 0) ? rate : rate * 1.13f;
    phase += effectiveRate / sampleRate;
    if (phase > 1.0f) phase -= 1.0f;

    // Dual LFO: primary sine + golden-ratio-offset secondary for richness
    float lfo1 = std::sin(2.0f * M_PI * phase);
    float lfo2 = std::sin(2.0f * M_PI * (phase * 1.618033989f));

    // Combined modulation: center delay 3ms + sweep ±1.5ms at max amount
    float centerDelaySamples = sampleRate * 0.003f; // 3ms
    float sweepDepth = sampleRate * 0.0015f * amount; // ±1.5ms at full amount
    float mod = (lfo1 * 0.7f + lfo2 * 0.3f) * sweepDepth;
    float readOffset = centerDelaySamples + mod;

    // Clamp to valid range
    readOffset = clamp(readOffset, 1.0f, static_cast<float>(BLOOM_BUFFER_SIZE - 4));

    // Catmull-Rom cubic interpolation (same pattern as DelayLine)
    float readPosF = static_cast<float>(writePos) - readOffset;
    if (readPosF < 0.0f) readPosF += BLOOM_BUFFER_SIZE;

    int idx0 = static_cast<int>(readPosF);
    float frac = readPosF - idx0;

    float y0 = buf[(idx0 - 1 + BLOOM_BUFFER_SIZE) & (BLOOM_BUFFER_SIZE - 1)];
    float y1 = buf[idx0 & (BLOOM_BUFFER_SIZE - 1)];
    float y2 = buf[(idx0 + 1) & (BLOOM_BUFFER_SIZE - 1)];
    float y3 = buf[(idx0 + 2) & (BLOOM_BUFFER_SIZE - 1)];

    float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float a2 = -0.5f * y0 + 0.5f * y2;
    float a3 = y1;

    float detuned = ((a0 * frac + a1) * frac + a2) * frac + a3;

    // Blend: dry + chorus wet
    return input * (1.0f - amount * 0.5f) + detuned * (amount * 0.5f);
}

// ===== QUANTIZE PITCH TO CURRENT SCALE =====
float CurveAndDragModule::quantizePitchToScale(float pitchCents) {
    if (params[MTS_ENABLE_PARAM].getValue() > 0.5f && mtsClient.isMtsConnected()) {
        return quantizePitchMTS(pitchCents);
    } else if (scalaReader.isLoaded()) {
        return quantizePitchScala(pitchCents);
    } else {
        int scaleIndex = clamp(static_cast<int>(params[SCALE_SELECT_PARAM].getValue()), 0, 10);
        return quantizePitchBuiltIn(pitchCents, scaleIndex);
    }
}

// ===== HELPER METHODS =====
void CurveAndDragModule::processDelayParameters(float sampleRate) {
    // Only set time from params when per-channel CV is not connected AND sync is off
    if (!params[SYNC_L_PARAM].getValue() && !inputs[TIME_L_CV_INPUT].isConnected()) {
        float leftTimeBase = params[TIME_L_PARAM].getValue() * 2000.0f;
        float leftTimeMs = leftTimeBase + (timeCVGlobal * 100.0f);
        leftDelay.setDelayTime(clamp(leftTimeMs, 1.0f, 2000.0f));
    }

    if (!params[SYNC_R_PARAM].getValue() && !inputs[TIME_R_CV_INPUT].isConnected()) {
        float rightTimeBase = params[TIME_R_PARAM].getValue() * 2000.0f;
        float rightTimeMs = rightTimeBase + (timeCVGlobal * 100.0f);
        rightDelay.setDelayTime(clamp(rightTimeMs, 1.0f, 2000.0f));
    }

    if (!inputs[FEEDBACK_L_CV_INPUT].isConnected()) {
        float leftFeedback = params[FEEDBACK_L_PARAM].getValue();
        leftFeedback += feedbackCVGlobal * 0.1f;
        leftDelay.setFeedback(clamp(leftFeedback, 0.0f, 1.1f));
    }

    if (!inputs[FEEDBACK_R_CV_INPUT].isConnected()) {
        float rightFeedback = params[FEEDBACK_R_PARAM].getValue();
        rightFeedback += feedbackCVGlobal * 0.1f;
        rightDelay.setFeedback(clamp(rightFeedback, 0.0f, 1.1f));
    }

    if (!inputs[MIX_L_CV_INPUT].isConnected()) {
        leftDelay.setDryWet(params[MIX_L_PARAM].getValue());
    }

    if (!inputs[MIX_R_CV_INPUT].isConnected()) {
        rightDelay.setDryWet(params[MIX_R_PARAM].getValue());
    }
}

void CurveAndDragModule::processTapTempo(float sampleRate) {
    if (leftTapTrigger.process(inputs[TAP_L_TRIGGER_INPUT].getVoltage())) {
        float currentTime = processCounter / sampleRate;
        float tapInterval = currentTime - lastTapTime;

        if (tapInterval > 0.25f && tapInterval < 2.0f) {
            detectedBPM = 60.0f / tapInterval;
            lastTapTime = currentTime;
            if (params[SYNC_L_PARAM].getValue() > 0.5f) {
                processTempo();
            }
        }
    }

    if (rightTapTrigger.process(inputs[TAP_R_TRIGGER_INPUT].getVoltage())) {
        float currentTime = processCounter / sampleRate;
        float tapInterval = currentTime - lastTapTime;

        if (tapInterval > 0.25f && tapInterval < 2.0f) {
            detectedBPM = 60.0f / tapInterval;
            lastTapTime = currentTime;
            if (params[SYNC_R_PARAM].getValue() > 0.5f) {
                processTempo();
            }
        }
    }
}

// ===== PITCH PROCESSING =====
void CurveAndDragModule::processPitchParameters() {
    float basePitch = getClampedParam(PITCH_PARAM, PITCH_CV_INPUT, -2.0f, 2.0f) * 1200.0f + pitchCVModulation;
    float detuneL = getClampedParam(DETUNE_L_PARAM, DETUNE_L_CV_INPUT, -1.0f, 1.0f) * 50.0f;
    float detuneR = getClampedParam(DETUNE_R_PARAM, DETUNE_R_CV_INPUT, -1.0f, 1.0f) * 50.0f;
    float detuneDrift = getClampedParam(DETUNE_DRIFT_PARAM, DETUNE_DRIFT_CV_INPUT, 0.0f, 1.0f) * 25.0f;

    float sr = APP->engine->getSampleRate();
    float smoothRate = 1.0f - std::exp(-1.0f / (sr * 0.005f));
    float detuneSmooth = 1.0f - std::exp(-1.0f / (sr * 0.002f));
    float driftSmooth = 1.0f - std::exp(-1.0f / (sr * 0.005f));
    smoothedBasePitch += (basePitch - smoothedBasePitch) * smoothRate;
    smoothedDetuneL += (detuneL - smoothedDetuneL) * detuneSmooth;
    smoothedDetuneR += (detuneR - smoothedDetuneR) * detuneSmooth;
    smoothedDrift += (detuneDrift - smoothedDrift) * driftSmooth;

    lastRawPitch = smoothedBasePitch;
    float quantizedBase = smoothedBasePitch;

    if (params[QUANTIZE_PARAM].getValue() > 0.5f) {
        quantizedBase = quantizePitchToScale(smoothedBasePitch);
    }

    lastQuantizedPitch = quantizedBase;
    lastDetuneL = quantizedBase + smoothedDetuneL + smoothedDrift;
    lastDetuneR = quantizedBase + smoothedDetuneR - smoothedDrift;
}

// ===== QUANTIZATION METHODS =====
float CurveAndDragModule::quantizePitchMTS(float pitchCents) {
    float semitones = pitchCents / 100.0f;
    int nearestMidi = clamp(static_cast<int>(std::round(60.0f + semitones)), 0, 127);

    if (mtsClient.shouldFilterNote(nearestMidi)) {
        return pitchCents;
    }

    double mtsFreq = mtsClient.getNoteFrequency(static_cast<float>(nearestMidi));
    double tetFreq = 440.0 * std::pow(2.0, (nearestMidi - 69.0) / 12.0);
    double mtsCentsOffset = 1200.0 * std::log2(mtsFreq / tetFreq);
    float baseCents = (nearestMidi - 60.0f) * 100.0f;
    return static_cast<float>(baseCents + mtsCentsOffset);
}

float CurveAndDragModule::quantizePitchScala(float pitchCents) {
    return scalaReader.quantizePitch(pitchCents);
}

float CurveAndDragModule::quantizePitchBuiltIn(float pitchCents, int scaleIndex) {
    switch (scaleIndex) {
        case 0: return std::round(pitchCents / 100.0f) * 100.0f;
        case 1: return std::round(pitchCents / 50.0f) * 50.0f;
        case 2: return std::round(pitchCents / (1200.0f / 31.0f)) * (1200.0f / 31.0f);
        case 3: {
            static const float justRatios[12] = {1.0f, 16.0f/15.0f, 9.0f/8.0f, 6.0f/5.0f, 5.0f/4.0f, 4.0f/3.0f,
                                                 45.0f/32.0f, 3.0f/2.0f, 8.0f/5.0f, 5.0f/3.0f, 9.0f/5.0f, 15.0f/8.0f};
            float semi = pitchCents / 100.0f;
            int octave = static_cast<int>(std::floor(semi / 12.0f));
            float fractional = semi - octave * 12.0f;
            int idx = static_cast<int>(std::round(fractional));
            if (idx >= 12) { idx = 0; octave++; }
            idx = clamp(idx, 0, 11);
            return 1200.0f * std::log2(justRatios[idx]) + octave * 1200.0f;
        }
        case 4: {
            static const float pythRatios[12] = {1.0f, 256.0f/243.0f, 9.0f/8.0f, 32.0f/27.0f, 81.0f/64.0f, 4.0f/3.0f,
                                                 729.0f/512.0f, 3.0f/2.0f, 128.0f/81.0f, 27.0f/16.0f, 16.0f/9.0f, 243.0f/128.0f};
            float semi = pitchCents / 100.0f;
            int octave = static_cast<int>(std::floor(semi / 12.0f));
            float fractional = semi - octave * 12.0f;
            int idx = static_cast<int>(std::round(fractional));
            if (idx >= 12) { idx = 0; octave++; }
            idx = clamp(idx, 0, 11);
            return 1200.0f * std::log2(pythRatios[idx]) + octave * 1200.0f;
        }
        case 5: {
            static const float meantoneSteps[12] = {0.0f, 76.0f, 193.0f, 310.0f, 386.0f, 503.0f, 579.0f,
                                                    697.0f, 773.0f, 890.0f, 1007.0f, 1083.0f};
            float semi = pitchCents / 100.0f;
            int octave = static_cast<int>(std::floor(semi / 12.0f));
            float fractional = semi - octave * 12.0f;
            int idx = static_cast<int>(std::round(fractional));
            if (idx >= 12) { idx = 0; octave++; }
            idx = clamp(idx, 0, 11);
            return meantoneSteps[idx] + octave * 1200.0f;
        }
        case 6: {
            static const float wellTempSteps[12] = {0.0f, 90.2f, 192.2f, 294.1f, 390.2f, 498.0f, 588.3f,
                                                    696.1f, 792.2f, 888.3f, 996.1f, 1092.2f};
            float semi = pitchCents / 100.0f;
            int octave = static_cast<int>(std::floor(semi / 12.0f));
            float fractional = semi - octave * 12.0f;
            int idx = static_cast<int>(std::round(fractional));
            if (idx >= 12) { idx = 0; octave++; }
            idx = clamp(idx, 0, 11);
            return wellTempSteps[idx] + octave * 1200.0f;
        }
        case 7: return std::round(pitchCents / (1200.0f / 19.0f)) * (1200.0f / 19.0f);
        case 8: return std::round(pitchCents / (1200.0f / 22.0f)) * (1200.0f / 22.0f);
        case 9: return std::round(pitchCents / (1200.0f / 53.0f)) * (1200.0f / 53.0f);
        case 10: return std::round(pitchCents / (1200.0f / 72.0f)) * (1200.0f / 72.0f);
        default: return pitchCents;
    }
}

void CurveAndDragModule::processScaleSelection() {
    bool mtsEnabled = params[MTS_ENABLE_PARAM].getValue() > 0.5f;

    if (mtsEnabled && mtsClient.isMtsConnected()) {
        tuningSource = "MTS-ESP";
        tuningInfo = mtsClient.getMtsTuningName();
    } else if (scalaReader.isLoaded()) {
        tuningSource = "Scala";
        tuningInfo = scalaReader.getDescription();
    } else {
        static const std::string scaleNames[11] = {
            "12-TET", "24-TET", "31-EDO", "Just Intonation",
            "Pythagorean", "Meantone", "Well-Tempered",
            "19-TET", "22-TET", "53-TET", "72-TET"
        };
        int scaleIndex = clamp(static_cast<int>(params[SCALE_SELECT_PARAM].getValue()), 0, 10);
        tuningSource = "Built-in";
        tuningInfo = scaleNames[scaleIndex];
    }
}

void CurveAndDragModule::updateLevelMeters(float leftInput, float rightInput) {
    leftLevelSmooth += (leftInput - leftLevelSmooth) * LEVEL_SMOOTH_RATE;
    rightLevelSmooth += (rightInput - rightLevelSmooth) * LEVEL_SMOOTH_RATE;

    for (int i = 0; i < 5; i++) {
        float threshold = (i + 1) * 0.2f;
        // GreenRedLight: index+0 = green, index+1 = red
        lights[LEVEL_LIGHTS_L_START + i * 2].setBrightness(leftLevelSmooth > threshold ? 1.0f : 0.0f);
        lights[LEVEL_LIGHTS_L_START + i * 2 + 1].setBrightness(leftLevelSmooth > threshold * 2.0f ? 1.0f : 0.0f);
        lights[LEVEL_LIGHTS_R_START + i * 2].setBrightness(rightLevelSmooth > threshold ? 1.0f : 0.0f);
        lights[LEVEL_LIGHTS_R_START + i * 2 + 1].setBrightness(rightLevelSmooth > threshold * 2.0f ? 1.0f : 0.0f);
    }
}

void CurveAndDragModule::updateStatusLights() {
    lights[SYNC_L_LIGHT].setBrightness(params[SYNC_L_PARAM].getValue());
    lights[SYNC_R_LIGHT].setBrightness(params[SYNC_R_PARAM].getValue());
    lights[CROSS_FEEDBACK_LIGHT].setBrightness(params[CROSS_FEEDBACK_PARAM].getValue());
    lights[QUANTIZE_LIGHT].setBrightness(params[QUANTIZE_PARAM].getValue());
    lights[TAPE_MODE_LIGHT].setBrightness(params[TAPE_MODE_PARAM].getValue());
    lights[MTS_ACTIVE_LIGHT].setBrightness(mtsClient.isMtsConnected() ? 1.0f : 0.0f);

    float morphAmount = getClampedParam(MORPH_PARAM, MORPH_CV_INPUT, 0.0f, 1.0f);
    lights[MORPH_LIGHT].setBrightness(morphAmount);

    // Shimmer + feedback lights
    lights[SHIMMER_LIGHT].setBrightness(pitchPlacement == PITCH_IN_LOOP ? 1.0f : 0.0f);
    lights[FREEZE_LIGHT].setBrightness(freezeActive ? 1.0f : 0.0f);
    lights[FREQ_SHIFT_LIGHT].setBrightness(feedbackProc.freqShiftMode ? 1.0f : 0.0f);
    lights[REVERSE_LIGHT].setBrightness(params[REVERSE_GRAIN_PARAM].getValue() > 0.01f ? params[REVERSE_GRAIN_PARAM].getValue() : 0.0f);
    lights[BLOOM_LIGHT].setBrightness(params[BLOOM_AMOUNT_PARAM].getValue());
}

float CurveAndDragModule::getClampedParam(int paramId, int cvInputId, float minVal, float maxVal, float defaultVal) {
    float paramValue = params[paramId].getValue();

    if (cvInputId >= 0 && inputs[cvInputId].isConnected()) {
        float cvValue = inputs[cvInputId].getVoltage() / 10.0f;
        float range = maxVal - minVal;
        paramValue += cvValue * range * 0.1f;
    }

    return clamp(paramValue, minVal, maxVal);
}

// ===== JSON SAVE/LOAD =====
json_t* CurveAndDragModule::dataToJson() {
    json_t* rootJ = json_object();

    json_object_set_new(rootJ, "tuningSource", json_string(tuningSource.c_str()));
    json_object_set_new(rootJ, "tuningInfo", json_string(tuningInfo.c_str()));
    json_object_set_new(rootJ, "detectedBPM", json_real(detectedBPM));
    json_object_set_new(rootJ, "pitchPlacement", json_integer(static_cast<int>(pitchPlacement)));

    // Save Scala file path for reload
    if (scalaReader.isLoaded() && !scalaReader.getScaleFilePath().empty()) {
        json_object_set_new(rootJ, "scalaFilePath", json_string(scalaReader.getScaleFilePath().c_str()));
    }
    // Per-head DSP parameters
    json_t* headPitchJ = json_array();
    json_t* headCutoffJ = json_array();
    json_t* headModeJ = json_array();
    json_t* headLevelJ = json_array();
    json_t* headPanJ = json_array();
    for (int i = 0; i < 4; i++) {
        json_array_append_new(headPitchJ, json_real(headPitchSemitones[i]));
        json_array_append_new(headCutoffJ, json_real(headFilterCutoff[i]));
        json_array_append_new(headModeJ, json_integer(headFilterMode[i]));
        json_array_append_new(headLevelJ, json_real(headLevel[i]));
        json_array_append_new(headPanJ, json_real(headPan[i]));
    }
    json_object_set_new(rootJ, "headPitchSemitones", headPitchJ);
    json_object_set_new(rootJ, "headFilterCutoff", headCutoffJ);
    json_object_set_new(rootJ, "headFilterMode", headModeJ);
    json_object_set_new(rootJ, "headLevel", headLevelJ);
    json_object_set_new(rootJ, "headPan", headPanJ);

    return rootJ;
}

void CurveAndDragModule::dataFromJson(json_t* rootJ) {
    json_t* tuningSourceJ = json_object_get(rootJ, "tuningSource");
    if (tuningSourceJ) tuningSource = json_string_value(tuningSourceJ);

    json_t* tuningInfoJ = json_object_get(rootJ, "tuningInfo");
    if (tuningInfoJ) tuningInfo = json_string_value(tuningInfoJ);

    json_t* bpmJ = json_object_get(rootJ, "detectedBPM");
    if (bpmJ) detectedBPM = json_real_value(bpmJ);

    json_t* placementJ = json_object_get(rootJ, "pitchPlacement");
    if (placementJ) pitchPlacement = static_cast<PitchPlacement>(json_integer_value(placementJ));

    // Restore Scala file
    json_t* scalaPathJ = json_object_get(rootJ, "scalaFilePath");
    if (scalaPathJ) {
        std::string scalaPath = json_string_value(scalaPathJ);
        if (!scalaPath.empty()) {
            scalaReader.loadScalaFile(scalaPath);
        }
    }

    // Per-head DSP parameters
    json_t* headPitchJ = json_object_get(rootJ, "headPitchSemitones");
    json_t* headCutoffJ = json_object_get(rootJ, "headFilterCutoff");
    json_t* headModeJ = json_object_get(rootJ, "headFilterMode");
    json_t* headLevelJ = json_object_get(rootJ, "headLevel");
    json_t* headPanJ = json_object_get(rootJ, "headPan");
    for (int i = 0; i < 4; i++) {
        if (headPitchJ) headPitchSemitones[i] = json_real_value(json_array_get(headPitchJ, i));
        if (headCutoffJ) headFilterCutoff[i] = json_real_value(json_array_get(headCutoffJ, i));
        if (headModeJ) headFilterMode[i] = json_integer_value(json_array_get(headModeJ, i));
        if (headLevelJ) headLevel[i] = json_real_value(json_array_get(headLevelJ, i));
        if (headPanJ) headPan[i] = json_real_value(json_array_get(headPanJ, i));
    }
}

// ===== REMAINING HELPER METHODS =====
void CurveAndDragModule::processTapeParameters() {
    if (params[TAPE_MODE_PARAM].getValue() > 0.5f) {
        float baseDelayTime = 100.0f;

        for (int ch = 0; ch < 2; ch++) {
            for (int head = 0; head < 4; head++) {
                float headDelayTime = baseDelayTime + (head * 50.0f);

                bool syncEnabled = (ch == 0) ? params[SYNC_L_PARAM].getValue() > 0.5f : params[SYNC_R_PARAM].getValue() > 0.5f;
                if (syncEnabled) {
                    int subdivIndex = (ch == 0) ? static_cast<int>(params[SUBDIV_L_PARAM].getValue()) : static_cast<int>(params[SUBDIV_R_PARAM].getValue());
                    SubdivisionType subdivType = static_cast<SubdivisionType>(clamp(subdivIndex, 0, 5));
                    float subdivMultiplier = getSubdivisionMultiplier(subdivType);
                    float beatTimeMs = (60.0f / detectedBPM) * 1000.0f;
                    headDelayTime = beatTimeMs * subdivMultiplier * (1.0f + head * 0.1f);
                }

                tapeProcessor.playHeads[ch][head].setDelayTime(clamp(headDelayTime, 1.0f, 2000.0f));
            }
        }
    }
}

float CurveAndDragModule::getSubdivisionTimeMs(SubdivisionType subdivision, float beatDurationMs) {
    switch (subdivision) {
        case SUBDIVISION_1_1:   return beatDurationMs * 4.0f;
        case SUBDIVISION_1_2:   return beatDurationMs * 2.0f;
        case SUBDIVISION_1_4:   return beatDurationMs;
        case SUBDIVISION_1_8:   return beatDurationMs * 0.5f;
        case SUBDIVISION_1_8T:  return beatDurationMs * 0.333f;
        case SUBDIVISION_1_16:  return beatDurationMs * 0.25f;
        default:                return beatDurationMs;
    }
}

std::string CurveAndDragModule::getSubdivisionName(SubdivisionType subdivision) {
    switch (subdivision) {
        case SUBDIVISION_1_1:   return "1/1";
        case SUBDIVISION_1_2:   return "1/2";
        case SUBDIVISION_1_4:   return "1/4";
        case SUBDIVISION_1_8:   return "1/8";
        case SUBDIVISION_1_8T:  return "1/8T";
        case SUBDIVISION_1_16:  return "1/16";
        default:                return "1/4";
    }
}

} // namespace CurveAndDrag

// Register the model with the plugin
plugin::Model* modelCurveAndDrag = createModel<CurveAndDrag::CurveAndDragModule, CurveAndDrag::CurveAndDragWidget>("CurveAndDrag");
