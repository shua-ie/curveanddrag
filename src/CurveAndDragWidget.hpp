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

        // Panel dimensions - 720x380px = 48HP
        box.size = Vec(720, 380);
        
        // ===== SECTION 1: INPUT + SYNC (0-120px) =====
        
        // Input Gain
        addParam(createParamCentered<RoundBlackKnob>(Vec(30, 80), module, CurveAndDragModule::INPUT_GAIN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(30, 125), module, CurveAndDragModule::INPUT_GAIN_CV_INPUT));
        
        // Audio inputs
        addInput(createInputCentered<PJ301MPort>(Vec(70, 80), module, CurveAndDragModule::LEFT_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(90, 80), module, CurveAndDragModule::RIGHT_INPUT));
        
        // Tempo Sync Controls - NO DUPLICATES
        addParam(createParamCentered<CKSS>(Vec(31, 129), module, CurveAndDragModule::SYNC_L_PARAM));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(31, 140), module, CurveAndDragModule::SYNC_L_LIGHT));
        
        addParam(createParamCentered<CKSS>(Vec(91, 129), module, CurveAndDragModule::SYNC_R_PARAM));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(91, 140), module, CurveAndDragModule::SYNC_R_LIGHT));
        
        // Subdivision controls
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(31, 155), module, CurveAndDragModule::SUBDIV_L_PARAM));
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(91, 155), module, CurveAndDragModule::SUBDIV_R_PARAM));
        
        // Main Pitch knob
        addParam(createParamCentered<RoundHugeBlackKnob>(Vec(60, 210), module, CurveAndDragModule::PITCH_PARAM));
        
        // Character knob
        addParam(createParamCentered<RoundBlackKnob>(Vec(60, 270), module, CurveAndDragModule::CHARACTER_PARAM));
        
        // CV inputs
        addInput(createInputCentered<PJ301MPort>(Vec(20, 320), module, CurveAndDragModule::TAP_L_TRIGGER_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(40, 320), module, CurveAndDragModule::TAP_R_TRIGGER_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(80, 320), module, CurveAndDragModule::PITCH_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(100, 320), module, CurveAndDragModule::CHARACTER_CV_INPUT));
        
        // Subdivision displays
        SubdivisionDisplay* leftSubdivDisplay = new SubdivisionDisplay(0);
        leftSubdivDisplay->box.pos = Vec(15, 175);
        leftSubdivDisplay->box.size = Vec(32, 12);
        leftSubdivDisplay->module = module;
        addChild(leftSubdivDisplay);
        
        SubdivisionDisplay* rightSubdivDisplay = new SubdivisionDisplay(1);
        rightSubdivDisplay->box.pos = Vec(75, 175);
        rightSubdivDisplay->box.size = Vec(32, 12);
        rightSubdivDisplay->module = module;
        addChild(rightSubdivDisplay);
        
        // ===== SECTION 2: DELAY ENGINE (120-240px) =====
        
        // Time controls
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(150, 80), module, CurveAndDragModule::TIME_L_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(210, 80), module, CurveAndDragModule::TIME_R_PARAM));
        
        // Feedback controls
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(150, 140), module, CurveAndDragModule::FEEDBACK_L_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(210, 140), module, CurveAndDragModule::FEEDBACK_R_PARAM));
        
        // Mix controls
        addParam(createParamCentered<RoundBlackKnob>(Vec(150, 200), module, CurveAndDragModule::MIX_L_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(Vec(210, 200), module, CurveAndDragModule::MIX_R_PARAM));
        
        // Cross-feedback
        addParam(createParamCentered<CKSS>(Vec(180, 244), module, CurveAndDragModule::CROSS_FEEDBACK_PARAM));
        addChild(createLightCentered<MediumLight<GreenLight>>(Vec(180, 255), module, CurveAndDragModule::CROSS_FEEDBACK_LIGHT));
        
        // Head Configuration
        addParam(createParamCentered<RoundBlackKnob>(Vec(180, 280), module, CurveAndDragModule::HEAD_SELECT_PARAM));
        
        // CV inputs
        addInput(createInputCentered<PJ301MPort>(Vec(130, 320), module, CurveAndDragModule::TIME_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(150, 320), module, CurveAndDragModule::FEEDBACK_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(170, 320), module, CurveAndDragModule::MIX_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(190, 320), module, CurveAndDragModule::FEEDBACK_R_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(210, 320), module, CurveAndDragModule::TIME_R_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(230, 320), module, CurveAndDragModule::MIX_R_CV_INPUT));
        
        // Global modulation inputs
        addInput(createInputCentered<PJ301MPort>(Vec(150, 350), module, CurveAndDragModule::TIME_MOD_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(180, 350), module, CurveAndDragModule::HEAD_SELECT_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(210, 350), module, CurveAndDragModule::FEEDBACK_MOD_INPUT));
        
        // ===== SECTION 3: PITCH SYSTEM (240-360px) =====
        
        // Detune controls
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(270, 80), module, CurveAndDragModule::DETUNE_L_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(330, 80), module, CurveAndDragModule::DETUNE_R_PARAM));
        
        // Detune drift
        addParam(createParamCentered<RoundBlackKnob>(Vec(300, 130), module, CurveAndDragModule::DETUNE_DRIFT_PARAM));
        
        // Morph
        addParam(createParamCentered<RoundBlackKnob>(Vec(300, 180), module, CurveAndDragModule::MORPH_PARAM));
        addChild(createLightCentered<MediumLight<YellowLight>>(Vec(300, 165), module, CurveAndDragModule::MORPH_LIGHT));
        
        // Pitch Mode selector
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(270, 230), module, CurveAndDragModule::PITCH_MODE_PARAM));
        
        // Quantize toggle
        addParam(createParamCentered<CKSS>(Vec(330, 230), module, CurveAndDragModule::QUANTIZE_PARAM));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(330, 241), module, CurveAndDragModule::QUANTIZE_LIGHT));
        
        // Scale selector
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(300, 280), module, CurveAndDragModule::SCALE_SELECT_PARAM));
        
        // MTS-ESP toggle
        addParam(createParamCentered<CKSS>(Vec(270, 310), module, CurveAndDragModule::MTS_ENABLE_PARAM));
        addChild(createLightCentered<MediumLight<GreenLight>>(Vec(270, 321), module, CurveAndDragModule::MTS_ACTIVE_LIGHT));
        
        // CV inputs
        addInput(createInputCentered<PJ301MPort>(Vec(260, 350), module, CurveAndDragModule::DETUNE_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(280, 350), module, CurveAndDragModule::DETUNE_DRIFT_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(300, 350), module, CurveAndDragModule::MORPH_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(320, 350), module, CurveAndDragModule::DETUNE_R_CV_INPUT));
        
        // Scale display
        ScaleDisplay* scaleDisplay = new ScaleDisplay();
        scaleDisplay->box.pos = Vec(270, 295);
        scaleDisplay->box.size = Vec(60, 14);
        scaleDisplay->module = module;
        addChild(scaleDisplay);
        
        // ===== SECTION 4: TAPE DSP (360-480px) =====
        
        // Tape enable
        addParam(createParamCentered<CKSS>(Vec(390, 80), module, CurveAndDragModule::TAPE_MODE_PARAM));
        addChild(createLightCentered<MediumLight<RedLight>>(Vec(390, 91), module, CurveAndDragModule::TAPE_MODE_LIGHT));
        
        // Tape controls
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(390, 130), module, CurveAndDragModule::SATURATION_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(450, 130), module, CurveAndDragModule::AGING_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(420, 180), module, CurveAndDragModule::INSTABILITY_PARAM));
        
        // Tape noise controls - FIXED: Proper 0-8% range
        addParam(createParamCentered<CKSS>(Vec(390, 230), module, CurveAndDragModule::TAPE_NOISE_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(Vec(450, 230), module, CurveAndDragModule::NOISE_AMOUNT_PARAM));
        
        // Head EQ controls
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(380, 280), module, CurveAndDragModule::HEAD_BUMP_FREQ_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(405, 280), module, CurveAndDragModule::HEAD_BUMP_GAIN_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(430, 280), module, CurveAndDragModule::ROLLOFF_FREQ_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(455, 280), module, CurveAndDragModule::ROLLOFF_RESONANCE_PARAM));
        
        // CV inputs
        addInput(createInputCentered<PJ301MPort>(Vec(380, 320), module, CurveAndDragModule::SATURATION_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(400, 320), module, CurveAndDragModule::AGING_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(420, 320), module, CurveAndDragModule::INSTABILITY_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(440, 320), module, CurveAndDragModule::TAPE_NOISE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(460, 320), module, CurveAndDragModule::NOISE_AMOUNT_CV_INPUT));
        
        // ===== SECTION 5: MODULATION (480-600px) =====
        
        // Wow controls
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(510, 80), module, CurveAndDragModule::WOW_RATE_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(570, 80), module, CurveAndDragModule::WOW_DEPTH_PARAM));
        
        // Flutter controls
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(510, 140), module, CurveAndDragModule::FLUTTER_RATE_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(570, 140), module, CurveAndDragModule::FLUTTER_DEPTH_PARAM));
        
        // Waveform selectors
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(530, 200), module, CurveAndDragModule::WOW_WAVEFORM_PARAM));
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(550, 200), module, CurveAndDragModule::FLUTTER_WAVEFORM_PARAM));
        
        // CV inputs
        addInput(createInputCentered<PJ301MPort>(Vec(500, 260), module, CurveAndDragModule::WOW_RATE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(520, 260), module, CurveAndDragModule::WOW_DEPTH_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(540, 260), module, CurveAndDragModule::FLUTTER_RATE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(560, 260), module, CurveAndDragModule::FLUTTER_DEPTH_CV_INPUT));
        
        // ===== SECTION 6: OUTPUT (600-720px) =====
        
        // Output gain
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(630, 80), module, CurveAndDragModule::OUTPUT_GAIN_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(630, 125), module, CurveAndDragModule::OUTPUT_GAIN_CV_INPUT));
        
        // Wet-only toggle (placeholder for future feature)
        // Note: This toggle is shown in SVG but not implemented in DSP yet
        // addParam(createParamCentered<CKSS>(Vec(690, 80), module, CurveAndDragModule::WET_ONLY_PARAM));
        
        // Main outputs
        addOutput(createOutputCentered<PJ301MPort>(Vec(650, 160), module, CurveAndDragModule::LEFT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(680, 160), module, CurveAndDragModule::RIGHT_OUTPUT));
        
        // Wet-only outputs
        addOutput(createOutputCentered<PJ301MPort>(Vec(650, 200), module, CurveAndDragModule::WET_LEFT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(680, 200), module, CurveAndDragModule::WET_RIGHT_OUTPUT));
        
        // Level meters
        for (int i = 0; i < 5; i++) {
            float meterY = 260 + i * 10;
            addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(650, meterY), module, CurveAndDragModule::LEVEL_LIGHTS_L_START + i));
            addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(680, meterY), module, CurveAndDragModule::LEVEL_LIGHTS_R_START + i));
        }
        
        // Tuning display
        TuningInfoDisplay* tuningDisplay = createWidget<TuningInfoDisplay>(Vec(620, 320));
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
                snprintf(rawPitchStr, sizeof(rawPitchStr), "Raw: %.1f¢", module->lastRawPitch);
                snprintf(quantPitchStr, sizeof(quantPitchStr), "Quant: %.1f¢", module->lastQuantizedPitch);
                
                nvgText(args.vg, box.size.x/2, 22, rawPitchStr, NULL);
                nvgText(args.vg, box.size.x/2, 28, quantPitchStr, NULL);
            }
        }
    };
    
    void appendContextMenu(Menu* menu) override {
        CurveAndDragModule* module = dynamic_cast<CurveAndDragModule*>(this->module);
        if (!module) return;
        
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("CurveAndDrag v2.8.1 STUDIO"));
        
        // Updated tooltip information
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Parameter Ranges:"));
        menu->addChild(createMenuLabel("• Tape Noise: 0-8% (Fixed)"));
        menu->addChild(createMenuLabel("• Head Config: 1-4 Heads"));
        menu->addChild(createMenuLabel("• Scale Select: 0-10 Built-in"));
        menu->addChild(createMenuLabel("• Pitch Algorithms: BBD/H910/Tape/Hybrid"));
        
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
            std::vector<std::string> modeNames = {"BBD (Analog)", "H910 (Harmonizer)", "Varispeed (Tape)", "Hybrid"};
            for (int i = 0; i < 4; i++) {
                subMenu->addChild(createMenuItem(modeNames[i], 
                    module->params[CurveAndDragModule::PITCH_MODE_PARAM].getValue() == i ? "✓" : "", 
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
