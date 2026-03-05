#pragma once
#include "CurveAndDrag.hpp"
#include "plugin.hpp"
#include <rack.hpp>
#include <osdialog.h>

namespace CurveAndDrag {

// ===== TEXT LABEL WIDGET FOR ANNOTATIONS =====
struct TextLabel : TransparentWidget {
    std::string text = "";
    int fontSize = 10;
    NVGcolor color = nvgRGBA(200, 200, 200, 255);
    std::shared_ptr<Font> font;

    TextLabel() {
        font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
        if (!font) {
            font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
        }
    }

    void draw(const DrawArgs &args) override {
        if (text.empty()) return;

        if (font) {
            nvgFontFaceId(args.vg, font->handle);
        }
        nvgFontSize(args.vg, fontSize);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, color);
        nvgText(args.vg, box.size.x/2, box.size.y/2, text.c_str(), NULL);
    }
};

struct CurveAndDragWidget : ModuleWidget {
    CurveAndDragWidget(CurveAndDragModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/CurveAndDrag.svg")));

        // Panel dimensions - 840x380px = 56HP
        box.size = Vec(840, 380);

        // ===== SECTION 1: INPUT + SYNC (0-140px) =====

        // Input Gain
        addParam(createParamCentered<RoundBlackKnob>(Vec(35, 80), module, CurveAndDragModule::INPUT_GAIN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(35, 110), module, CurveAndDragModule::INPUT_GAIN_CV_INPUT));

        // Audio inputs
        addInput(createInputCentered<PJ301MPort>(Vec(82, 80), module, CurveAndDragModule::LEFT_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(108, 80), module, CurveAndDragModule::RIGHT_INPUT));

        // Tempo Sync Controls
        addParam(createParamCentered<CKSS>(Vec(35, 140), module, CurveAndDragModule::SYNC_L_PARAM));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(35, 152), module, CurveAndDragModule::SYNC_L_LIGHT));

        addParam(createParamCentered<CKSS>(Vec(108, 140), module, CurveAndDragModule::SYNC_R_PARAM));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(108, 152), module, CurveAndDragModule::SYNC_R_LIGHT));

        // Subdivision controls
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(35, 172), module, CurveAndDragModule::SUBDIV_L_PARAM));
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(108, 172), module, CurveAndDragModule::SUBDIV_R_PARAM));

        // Main Pitch knob
        addParam(createParamCentered<RoundHugeBlackKnob>(Vec(70, 225), module, CurveAndDragModule::PITCH_PARAM));

        // Character knob
        addParam(createParamCentered<RoundBlackKnob>(Vec(70, 280), module, CurveAndDragModule::CHARACTER_PARAM));

        // Bottom CV row
        addInput(createInputCentered<PJ301MPort>(Vec(22, 320), module, CurveAndDragModule::TAP_L_TRIGGER_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(48, 320), module, CurveAndDragModule::TAP_R_TRIGGER_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(70, 320), module, CurveAndDragModule::PITCH_VOCT_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(92, 320), module, CurveAndDragModule::PITCH_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(118, 320), module, CurveAndDragModule::CHARACTER_CV_INPUT));

        // Subdivision displays
        SubdivisionDisplay* leftSubdivDisplay = new SubdivisionDisplay(0);
        leftSubdivDisplay->box.pos = Vec(20, 186);
        leftSubdivDisplay->box.size = Vec(32, 12);
        leftSubdivDisplay->module = module;
        addChild(leftSubdivDisplay);

        SubdivisionDisplay* rightSubdivDisplay = new SubdivisionDisplay(1);
        rightSubdivDisplay->box.pos = Vec(92, 186);
        rightSubdivDisplay->box.size = Vec(32, 12);
        rightSubdivDisplay->module = module;
        addChild(rightSubdivDisplay);

        // ===== SECTION 2: DELAY ENGINE (140-280px) =====

        // Time controls
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(175, 80), module, CurveAndDragModule::TIME_L_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(245, 80), module, CurveAndDragModule::TIME_R_PARAM));

        // Feedback controls
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(175, 140), module, CurveAndDragModule::FEEDBACK_L_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(245, 140), module, CurveAndDragModule::FEEDBACK_R_PARAM));

        // Mix controls
        addParam(createParamCentered<RoundBlackKnob>(Vec(175, 200), module, CurveAndDragModule::MIX_L_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(Vec(245, 200), module, CurveAndDragModule::MIX_R_PARAM));

        // Cross-feedback
        addParam(createParamCentered<CKSS>(Vec(210, 244), module, CurveAndDragModule::CROSS_FEEDBACK_PARAM));
        addChild(createLightCentered<MediumLight<GreenLight>>(Vec(210, 255), module, CurveAndDragModule::CROSS_FEEDBACK_LIGHT));

        // Head Configuration
        addParam(createParamCentered<RoundBlackKnob>(Vec(210, 280), module, CurveAndDragModule::HEAD_SELECT_PARAM));

        // CV inputs
        addInput(createInputCentered<PJ301MPort>(Vec(152, 320), module, CurveAndDragModule::TIME_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(175, 320), module, CurveAndDragModule::FEEDBACK_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(198, 320), module, CurveAndDragModule::MIX_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(222, 320), module, CurveAndDragModule::FEEDBACK_R_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(245, 320), module, CurveAndDragModule::TIME_R_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(268, 320), module, CurveAndDragModule::MIX_R_CV_INPUT));

        // Global modulation inputs
        addInput(createInputCentered<PJ301MPort>(Vec(175, 350), module, CurveAndDragModule::TIME_MOD_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(245, 350), module, CurveAndDragModule::FEEDBACK_MOD_INPUT));

        // ===== SECTION 3: PITCH SYSTEM (280-420px) =====

        // Detune controls
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(315, 80), module, CurveAndDragModule::DETUNE_L_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(385, 80), module, CurveAndDragModule::DETUNE_R_PARAM));

        // Detune drift
        addParam(createParamCentered<RoundBlackKnob>(Vec(350, 130), module, CurveAndDragModule::DETUNE_DRIFT_PARAM));

        // Morph
        addParam(createParamCentered<RoundBlackKnob>(Vec(350, 175), module, CurveAndDragModule::MORPH_PARAM));
        addChild(createLightCentered<MediumLight<YellowLight>>(Vec(350, 160), module, CurveAndDragModule::MORPH_LIGHT));

        // Pitch Mode selector
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(315, 220), module, CurveAndDragModule::PITCH_MODE_PARAM));

        // Quantize toggle
        addParam(createParamCentered<CKSS>(Vec(385, 220), module, CurveAndDragModule::QUANTIZE_PARAM));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(385, 232), module, CurveAndDragModule::QUANTIZE_LIGHT));

        // Independent L/R pitch
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(305, 260), module, CurveAndDragModule::PITCH_L_PARAM));

        // Scale selector
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(350, 260), module, CurveAndDragModule::SCALE_SELECT_PARAM));

        // Independent R pitch
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(395, 260), module, CurveAndDragModule::PITCH_R_PARAM));

        // MTS-ESP toggle
        addParam(createParamCentered<CKSS>(Vec(310, 289), module, CurveAndDragModule::MTS_ENABLE_PARAM));
        addChild(createLightCentered<MediumLight<GreenLight>>(Vec(310, 301), module, CurveAndDragModule::MTS_ACTIVE_LIGHT));

        // Time stretch (spectral mode)
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(390, 289), module, CurveAndDragModule::TIME_STRETCH_PARAM));

        // Scale display
        ScaleDisplay* scaleDisplay = new ScaleDisplay();
        scaleDisplay->box.pos = Vec(330, 264);
        scaleDisplay->box.size = Vec(40, 12);
        scaleDisplay->module = module;
        addChild(scaleDisplay);

        // CV inputs row 1
        addInput(createInputCentered<PJ301MPort>(Vec(296, 334), module, CurveAndDragModule::DETUNE_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(322, 334), module, CurveAndDragModule::DETUNE_DRIFT_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(348, 334), module, CurveAndDragModule::MORPH_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(374, 334), module, CurveAndDragModule::DETUNE_R_CV_INPUT));

        // CV inputs row 2
        addInput(createInputCentered<PJ301MPort>(Vec(296, 360), module, CurveAndDragModule::PITCH_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(348, 360), module, CurveAndDragModule::PITCH_R_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(400, 360), module, CurveAndDragModule::TIME_STRETCH_CV_INPUT));

        // ===== SECTION 4: SHIMMER + FEEDBACK (420-560px) =====
        // 11 rows evenly spaced: y=60,88,116,144,172,200,228,256,284,312,340

        // Row 1 (y=60): Pitch placement + shimmer quantize toggles
        addParam(createParamCentered<CKSS>(Vec(440, 60), module, CurveAndDragModule::PITCH_PLACEMENT_PARAM));
        addChild(createLightCentered<MediumLight<YellowLight>>(Vec(440, 48), module, CurveAndDragModule::SHIMMER_LIGHT));
        addParam(createParamCentered<CKSS>(Vec(468, 60), module, CurveAndDragModule::SHIMMER_QUANTIZE_PARAM));

        // Row 2 (y=88): Shimmer pitch, CV, direction, mix
        addParam(createParamCentered<RoundBlackKnob>(Vec(445, 88), module, CurveAndDragModule::SHIMMER_PITCH_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(475, 88), module, CurveAndDragModule::SHIMMER_PITCH_CV_INPUT));
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(515, 88), module, CurveAndDragModule::PITCH_DIRECTION_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(545, 88), module, CurveAndDragModule::SHIMMER_MIX_PARAM));

        // Row 3 (y=116): Feedback filters
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(435, 116), module, CurveAndDragModule::FB_LP_FREQ_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(465, 116), module, CurveAndDragModule::FB_HP_FREQ_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(495, 116), module, CurveAndDragModule::FB_TILT_PARAM));

        // Row 4 (y=144): Filter CVs
        addInput(createInputCentered<PJ301MPort>(Vec(435, 144), module, CurveAndDragModule::FB_LP_FREQ_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(465, 144), module, CurveAndDragModule::FB_HP_FREQ_CV_INPUT));

        // Row 5 (y=172): Drive, compression, ducking
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(440, 172), module, CurveAndDragModule::FB_DRIVE_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(475, 172), module, CurveAndDragModule::FB_COMP_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(510, 172), module, CurveAndDragModule::DUCK_AMOUNT_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(545, 172), module, CurveAndDragModule::DUCK_RELEASE_PARAM));

        // Row 6 (y=200): Duck CV
        addInput(createInputCentered<PJ301MPort>(Vec(510, 200), module, CurveAndDragModule::DUCK_AMOUNT_CV_INPUT));

        // Row 7 (y=228): Pitch drift, freeze
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(435, 228), module, CurveAndDragModule::PITCH_DRIFT_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(465, 228), module, CurveAndDragModule::DRIFT_RATE_PARAM));
        addParam(createParamCentered<CKSS>(Vec(500, 228), module, CurveAndDragModule::FREEZE_PARAM));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(500, 218), module, CurveAndDragModule::FREEZE_LIGHT));
        addInput(createInputCentered<PJ301MPort>(Vec(530, 228), module, CurveAndDragModule::FREEZE_CV_INPUT));

        // Row 8 (y=256): Frequency shift
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(435, 256), module, CurveAndDragModule::FREQ_SHIFT_PARAM));
        addParam(createParamCentered<CKSS>(Vec(465, 256), module, CurveAndDragModule::FREQ_SHIFT_MODE_PARAM));
        addChild(createLightCentered<MediumLight<GreenLight>>(Vec(465, 246), module, CurveAndDragModule::FREQ_SHIFT_LIGHT));
        addInput(createInputCentered<PJ301MPort>(Vec(495, 256), module, CurveAndDragModule::FREQ_SHIFT_CV_INPUT));

        // Row 9 (y=284): Reverse granular
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(435, 284), module, CurveAndDragModule::REVERSE_GRAIN_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(465, 284), module, CurveAndDragModule::REVERSE_GRAIN_SIZE_PARAM));
        addChild(createLightCentered<MediumLight<YellowLight>>(Vec(450, 274), module, CurveAndDragModule::REVERSE_LIGHT));
        addInput(createInputCentered<PJ301MPort>(Vec(498, 284), module, CurveAndDragModule::REVERSE_GRAIN_CV_INPUT));

        // Row 10 (y=312): Bloom
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(435, 312), module, CurveAndDragModule::BLOOM_AMOUNT_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(465, 312), module, CurveAndDragModule::BLOOM_RATE_PARAM));
        addChild(createLightCentered<MediumLight<GreenLight>>(Vec(450, 302), module, CurveAndDragModule::BLOOM_LIGHT));
        addInput(createInputCentered<PJ301MPort>(Vec(498, 312), module, CurveAndDragModule::BLOOM_CV_INPUT));

        // Row 11 (y=340): Pitch envelope
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(435, 340), module, CurveAndDragModule::PITCH_ENV_AMOUNT_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(465, 340), module, CurveAndDragModule::PITCH_ENV_SPEED_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(498, 340), module, CurveAndDragModule::PITCH_ENV_CV_INPUT));

        // ===== SECTION 5: TAPE DSP (560-700px) =====

        // Tape enable
        addParam(createParamCentered<CKSS>(Vec(630, 62), module, CurveAndDragModule::TAPE_MODE_PARAM));
        addChild(createLightCentered<MediumLight<RedLight>>(Vec(630, 50), module, CurveAndDragModule::TAPE_MODE_LIGHT));

        // Saturation and Aging
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(595, 90), module, CurveAndDragModule::SATURATION_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(665, 90), module, CurveAndDragModule::AGING_PARAM));

        // Instability
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(630, 140), module, CurveAndDragModule::INSTABILITY_PARAM));

        // Noise controls
        addParam(createParamCentered<CKSS>(Vec(595, 180), module, CurveAndDragModule::TAPE_NOISE_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(Vec(640, 180), module, CurveAndDragModule::NOISE_AMOUNT_PARAM));

        // Head EQ controls
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(585, 215), module, CurveAndDragModule::HEAD_BUMP_FREQ_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(612, 215), module, CurveAndDragModule::HEAD_BUMP_GAIN_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(648, 215), module, CurveAndDragModule::ROLLOFF_FREQ_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(675, 215), module, CurveAndDragModule::ROLLOFF_RESONANCE_PARAM));

        // Wow controls
        addParam(createParamCentered<RoundBlackKnob>(Vec(590, 250), module, CurveAndDragModule::WOW_RATE_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(Vec(630, 250), module, CurveAndDragModule::WOW_DEPTH_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(670, 250), module, CurveAndDragModule::WOW_WAVEFORM_PARAM));

        // Flutter controls
        addParam(createParamCentered<RoundBlackKnob>(Vec(590, 285), module, CurveAndDragModule::FLUTTER_RATE_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(Vec(630, 285), module, CurveAndDragModule::FLUTTER_DEPTH_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(670, 285), module, CurveAndDragModule::FLUTTER_WAVEFORM_PARAM));

        // Tape CV inputs row 1
        addInput(createInputCentered<PJ301MPort>(Vec(580, 320), module, CurveAndDragModule::SATURATION_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(606, 320), module, CurveAndDragModule::AGING_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(632, 320), module, CurveAndDragModule::INSTABILITY_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(658, 320), module, CurveAndDragModule::TAPE_NOISE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(684, 320), module, CurveAndDragModule::NOISE_AMOUNT_CV_INPUT));

        // Tape CV inputs row 2
        addInput(createInputCentered<PJ301MPort>(Vec(580, 350), module, CurveAndDragModule::WOW_RATE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(606, 350), module, CurveAndDragModule::WOW_DEPTH_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(632, 350), module, CurveAndDragModule::FLUTTER_RATE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(658, 350), module, CurveAndDragModule::FLUTTER_DEPTH_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(684, 350), module, CurveAndDragModule::HEAD_SELECT_CV_INPUT));

        // ===== SECTION 6: OUTPUT (700-840px) =====

        // Output gain
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(770, 80), module, CurveAndDragModule::OUTPUT_GAIN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(770, 120), module, CurveAndDragModule::OUTPUT_GAIN_CV_INPUT));

        // Main outputs
        addOutput(createOutputCentered<PJ301MPort>(Vec(745, 160), module, CurveAndDragModule::LEFT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(795, 160), module, CurveAndDragModule::RIGHT_OUTPUT));

        // Wet-only outputs
        addOutput(createOutputCentered<PJ301MPort>(Vec(745, 200), module, CurveAndDragModule::WET_LEFT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(795, 200), module, CurveAndDragModule::WET_RIGHT_OUTPUT));

        // Level meters — GreenRedLight uses 2 light IDs per instance
        for (int i = 0; i < 5; i++) {
            float meterY = 260 + i * 10;
            addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(755, meterY), module, CurveAndDragModule::LEVEL_LIGHTS_L_START + i * 2));
            addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(785, meterY), module, CurveAndDragModule::LEVEL_LIGHTS_R_START + i * 2));
        }

        // Tuning display
        TuningInfoDisplay* tuningDisplay = createWidget<TuningInfoDisplay>(Vec(725, 310));
        tuningDisplay->box.size = Vec(90, 35);
        tuningDisplay->module = module;
        addChild(tuningDisplay);
    }

    // ===== SUBDIVISION DISPLAY WIDGET =====
    struct SubdivisionDisplay : TransparentWidget {
        CurveAndDragModule *module;
        int channel; // 0=left, 1=right
        std::shared_ptr<Font> font;

        SubdivisionDisplay(int ch) : channel(ch) {
            font = APP->window->loadFont(asset::system("res/fonts/DSEG7ClassicMini-BoldItalic.ttf"));
            if (!font) {
                font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
            }
            if (!font) {
                font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
            }
        }

        void draw(const DrawArgs &args) override {
            if (!module) return;

            bool syncEnabled = (channel == 0) ?
                module->params[CurveAndDragModule::SYNC_L_PARAM].getValue() > 0.5f :
                module->params[CurveAndDragModule::SYNC_R_PARAM].getValue() > 0.5f;

            // Background
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);

            if (syncEnabled) {
                nvgFillColor(args.vg, nvgRGBA(20, 40, 80, 240));
                nvgStrokeColor(args.vg, nvgRGBA(100, 150, 255, 255));
            } else {
                nvgFillColor(args.vg, nvgRGBA(20, 20, 25, 180));
                nvgStrokeColor(args.vg, nvgRGBA(60, 60, 70, 150));
            }
            nvgFill(args.vg);
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);

            // Text
            if (font) {
                nvgFontFaceId(args.vg, font->handle);
            }
            nvgFontSize(args.vg, 8);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

            std::string displayText;
            NVGcolor textColor;

            if (syncEnabled) {
                int subdivParam = (channel == 0) ?
                    CurveAndDragModule::SUBDIV_L_PARAM : CurveAndDragModule::SUBDIV_R_PARAM;
                int subdivIndex = static_cast<int>(module->params[subdivParam].getValue());
                CurveAndDragModule::SubdivisionType subdivType =
                    static_cast<CurveAndDragModule::SubdivisionType>(clamp(subdivIndex, 0, 5));

                displayText = module->getSubdivisionName(subdivType);
                textColor = nvgRGBA(150, 200, 255, 255);
            } else {
                displayText = "FREE";
                textColor = nvgRGBA(120, 120, 130, 200);
            }

            nvgFillColor(args.vg, textColor);
            nvgText(args.vg, box.size.x/2, box.size.y/2, displayText.c_str(), NULL);
        }
    };

    // ===== SCALE DISPLAY WIDGET =====
    struct ScaleDisplay : TransparentWidget {
        CurveAndDragModule *module;
        std::shared_ptr<Font> font;

        ScaleDisplay() {
            font = APP->window->loadFont(asset::system("res/fonts/DSEG7ClassicMini-BoldItalic.ttf"));
            if (!font) {
                font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
            }
            if (!font) {
                font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
            }
        }

        void draw(const DrawArgs &args) override {
            if (!module) return;

            int scaleIndex = clamp(static_cast<int>(module->params[CurveAndDragModule::SCALE_SELECT_PARAM].getValue()), 0, 10);

            std::vector<std::string> scaleNames = {
                "12-TET", "24-TET", "31-EDO", "Just", "Pythagorean", "Meantone",
                "Well-Temp", "19-TET", "22-TET", "53-TET", "72-TET"
            };

            std::string scaleName = scaleNames[scaleIndex];

            // Background
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);

            bool quantizeActive = module->params[CurveAndDragModule::QUANTIZE_PARAM].getValue() > 0.5f;
            bool mtsActive = module->params[CurveAndDragModule::MTS_ENABLE_PARAM].getValue() > 0.5f;

            if (quantizeActive) {
                if (mtsActive) {
                    nvgFillColor(args.vg, nvgRGBA(20, 60, 20, 240));
                    nvgStrokeColor(args.vg, nvgRGBA(100, 255, 100, 255));
                } else {
                    nvgFillColor(args.vg, nvgRGBA(20, 30, 60, 240));
                    nvgStrokeColor(args.vg, nvgRGBA(120, 180, 255, 255));
                }
            } else {
                nvgFillColor(args.vg, nvgRGBA(25, 25, 30, 200));
                nvgStrokeColor(args.vg, nvgRGBA(80, 80, 90, 180));
            }

            nvgFill(args.vg);
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);

            // Text
            if (font) {
                nvgFontFaceId(args.vg, font->handle);
            }
            nvgFontSize(args.vg, 8);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);

            NVGcolor textColor;

            if (!quantizeActive) {
                textColor = nvgRGBA(120, 120, 130, 180);
            } else if (mtsActive) {
                textColor = nvgRGBA(150, 255, 150, 255);
                scaleName = "MTS-ESP";
            } else {
                if (scaleName.find("TET") != std::string::npos || scaleName.find("EDO") != std::string::npos) {
                    textColor = nvgRGBA(150, 255, 150, 255);
                } else if (scaleName == "Just" || scaleName == "Pythagorean") {
                    textColor = nvgRGBA(255, 200, 100, 255);
                } else {
                    textColor = nvgRGBA(180, 220, 255, 255);
                }
            }

            nvgFillColor(args.vg, textColor);
            nvgText(args.vg, box.size.x/2, box.size.y/2, scaleName.c_str(), NULL);
        }
    };

    // ===== TUNING INFO DISPLAY =====
    struct TuningInfoDisplay : TransparentWidget {
        CurveAndDragModule *module;
        std::shared_ptr<Font> font;

        TuningInfoDisplay() {
            font = APP->window->loadFont(asset::system("res/fonts/DSEG7ClassicMini-BoldItalic.ttf"));
            if (!font) {
                font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
            }
        }

        void draw(const DrawArgs &args) override {
            if (!module) return;

            // Background
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
            nvgFillColor(args.vg, nvgRGBA(20, 20, 25, 240));
            nvgFill(args.vg);
            nvgStrokeColor(args.vg, nvgRGBA(100, 100, 120, 180));
            nvgStrokeWidth(args.vg, 0.75f);
            nvgStroke(args.vg);

            // Text
            nvgFontSize(args.vg, 7);
            if (font) {
                nvgFontFaceId(args.vg, font->handle);
            }
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);

            NVGcolor sourceColor = nvgRGBA(200, 200, 200, 255);
            if (module->tuningSource == "MTS-ESP") {
                sourceColor = nvgRGBA(100, 255, 100, 255);
            } else if (module->tuningSource == "Scala") {
                sourceColor = nvgRGBA(255, 255, 100, 255);
            } else {
                sourceColor = nvgRGBA(150, 200, 255, 255);
            }

            nvgFillColor(args.vg, sourceColor);
            nvgText(args.vg, box.size.x/2, 4, module->tuningSource.c_str(), NULL);

            nvgFillColor(args.vg, nvgRGBA(220, 220, 220, 200));
            nvgFontSize(args.vg, 5);
            nvgText(args.vg, box.size.x/2, 14, module->tuningInfo.c_str(), NULL);

            // Pitch info if quantization is active
            if (module->params[CurveAndDragModule::QUANTIZE_PARAM].getValue() > 0.5f) {
                nvgFontSize(args.vg, 4);
                nvgFillColor(args.vg, nvgRGBA(200, 255, 200, 180));

                char rawPitchStr[32];
                char quantPitchStr[32];
                snprintf(rawPitchStr, sizeof(rawPitchStr), "Raw: %.1f\xC2\xA2", module->lastRawPitch);
                snprintf(quantPitchStr, sizeof(quantPitchStr), "Quant: %.1f\xC2\xA2", module->lastQuantizedPitch);

                nvgText(args.vg, box.size.x/2, 22, rawPitchStr, NULL);
                nvgText(args.vg, box.size.x/2, 28, quantPitchStr, NULL);
            }
        }
    };

    void appendContextMenu(Menu* menu) override {
        CurveAndDragModule* module = dynamic_cast<CurveAndDragModule*>(this->module);
        if (!module) return;

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("CurveAndDrag v1.0"));

        // Parameter info
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Parameter Ranges:"));
        menu->addChild(createMenuLabel("  Tape Noise: 0-8% (Fixed)"));
        menu->addChild(createMenuLabel("  Head Config: 1-4 Heads"));
        menu->addChild(createMenuLabel("  Scale Select: 0-10 Built-in"));
        menu->addChild(createMenuLabel("  Pitch Algorithms: Lo-Fi/H910/Varispeed/Hybrid/Spectral"));

        // Scale loading options
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Tuning & Scales"));

        menu->addChild(createMenuItem("Load Scala File...", "", [=]() {
            std::string path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, osdialog_filters_parse("Scala Scale:scl"));
            if (!path.empty()) {
                module->scalaReader.loadScalaFile(path);
            }
        }));

        // Pitch mode submenu
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Pitch Algorithms"));

        menu->addChild(createSubmenuItem("Algorithm Selection", "", [=](Menu* subMenu) {
            std::vector<std::string> modeNames = {
                "Lo-Fi (Vintage)", "H910 (Harmonizer)", "Varispeed (Tape)",
                "Hybrid (Phase-Lock)", "Spectral (FFT)"
            };
            for (int i = 0; i < 5; i++) {
                subMenu->addChild(createMenuItem(modeNames[i],
                    module->params[CurveAndDragModule::PITCH_MODE_PARAM].getValue() == i ? "\xe2\x9c\x93" : "",
                    [=]() {
                        module->params[CurveAndDragModule::PITCH_MODE_PARAM].setValue(i);
                    }));
            }
        }));

        // Performance options
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Performance"));

        menu->addChild(createMenuItem("Reset All Parameters", "", [=]() {
            module->onReset();
        }));
    }
};

} // namespace CurveAndDrag
