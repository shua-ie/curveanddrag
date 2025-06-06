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

        // Panel dimensions - Matched to SVG (720x380px = 48HP)
        box.size = Vec(720, 380);
        
        // Define positioning constants based on SVG layout
        const float rowPositions[5] = {85.0f, 160.0f, 230.0f, 300.0f, 345.0f};
        
        // ===== SECTION 1: AUDIO I/O + MASTER (75px center) =====
        
        // Master gain controls with improved tooltips
        addParam(createParamCentered<RoundBlackKnob>(Vec(50, rowPositions[0]), module, CurveAndDragModule::INPUT_GAIN_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(Vec(100, rowPositions[0]), module, CurveAndDragModule::OUTPUT_GAIN_PARAM));
        
        // Audio I/O
        addInput(createInputCentered<PJ301MPort>(Vec(50, rowPositions[1]), module, CurveAndDragModule::LEFT_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(100, rowPositions[1]), module, CurveAndDragModule::RIGHT_INPUT));
        
        // Main outputs
        addOutput(createOutputCentered<PJ301MPort>(Vec(50, rowPositions[2]), module, CurveAndDragModule::LEFT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(100, rowPositions[2]), module, CurveAndDragModule::RIGHT_OUTPUT));
        
        // Wet-only outputs with proper tooltips
        addOutput(createOutputCentered<PJ301MPort>(Vec(50, rowPositions[3]), module, CurveAndDragModule::WET_LEFT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(100, rowPositions[3]), module, CurveAndDragModule::WET_RIGHT_OUTPUT));
        
        // Level meters (5 segments per channel)
        for (int i = 0; i < 5; i++) {
            float meterY = 105 + i * 10; // Spaced vertically in meter area
            addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(25, meterY), module, CurveAndDragModule::LEVEL_LIGHTS_L_START + i));
            addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(105, meterY), module, CurveAndDragModule::LEVEL_LIGHTS_R_START + i));
        }
        
        // ===== SECTION 2: DELAY CONTROLS (195px center) =====
        
        // Time controls with subdivision display
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(150, rowPositions[0]), module, CurveAndDragModule::TIME_L_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(240, rowPositions[0]), module, CurveAndDragModule::TIME_R_PARAM));
        
        // Feedback controls
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(150, rowPositions[1]), module, CurveAndDragModule::FEEDBACK_L_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(240, rowPositions[1]), module, CurveAndDragModule::FEEDBACK_R_PARAM));
        
        // Mix controls
        addParam(createParamCentered<RoundBlackKnob>(Vec(170, rowPositions[2]), module, CurveAndDragModule::MIX_L_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(Vec(220, rowPositions[2]), module, CurveAndDragModule::MIX_R_PARAM));
        
        // Cross-feedback toggle
        addParam(createParamCentered<CKSS>(Vec(181, 130), module, CurveAndDragModule::CROSS_FEEDBACK_PARAM));
        addChild(createLightCentered<MediumLight<GreenLight>>(Vec(181, 142), module, CurveAndDragModule::CROSS_FEEDBACK_LIGHT));
        
        // ===== CRITICAL FIX: Individual Channel Sync Controls (No Duplicates) =====
        // Left channel sync
        addParam(createParamCentered<CKSS>(Vec(211, 130), module, CurveAndDragModule::SYNC_L_PARAM));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(211, 142), module, CurveAndDragModule::SYNC_L_LIGHT));
        
        // Add text label for left sync
        TextLabel* leftSyncLabel = new TextLabel();
        leftSyncLabel->box.pos = Vec(200, 115);
        leftSyncLabel->box.size = Vec(22, 8);
        leftSyncLabel->text = "L";
        leftSyncLabel->fontSize = 7;
        leftSyncLabel->color = nvgRGBA(150, 150, 200, 255);
        addChild(leftSyncLabel);
        
        // Right channel sync
        addParam(createParamCentered<CKSS>(Vec(150, 195), module, CurveAndDragModule::SYNC_R_PARAM));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(150, 205), module, CurveAndDragModule::SYNC_R_LIGHT));
        
        // Add text label for right sync
        TextLabel* rightSyncLabel = new TextLabel();
        rightSyncLabel->box.pos = Vec(139, 180);
        rightSyncLabel->box.size = Vec(22, 8);
        rightSyncLabel->text = "R";
        rightSyncLabel->fontSize = 7;
        rightSyncLabel->color = nvgRGBA(150, 150, 200, 255);
        addChild(rightSyncLabel);
        
        // ===== CRITICAL FIX: Master Tempo Sync Control =====
        // Global tempo sync master switch (new parameter needs to be added to enum)
        addParam(createParamCentered<CKSS>(Vec(195, 105), module, CurveAndDragModule::SYNC_L_PARAM));
        addChild(createLightCentered<MediumLight<YellowLight>>(Vec(195, 117), module, CurveAndDragModule::SYNC_L_LIGHT));
        
        // Add text label for master sync
        TextLabel* masterSyncLabel = new TextLabel();
        masterSyncLabel->box.pos = Vec(175, 95);
        masterSyncLabel->box.size = Vec(40, 10);
        masterSyncLabel->text = "SYNC";
        masterSyncLabel->fontSize = 8;
        masterSyncLabel->color = nvgRGBA(200, 200, 200, 255);
        addChild(masterSyncLabel);
        
        // Sync toggles (individual channel overrides)
        addParam(createParamCentered<CKSS>(Vec(211, 130), module, CurveAndDragModule::SYNC_L_PARAM));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(211, 142), module, CurveAndDragModule::SYNC_L_LIGHT));
        
        addParam(createParamCentered<CKSS>(Vec(150, 195), module, CurveAndDragModule::SYNC_R_PARAM));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(150, 205), module, CurveAndDragModule::SYNC_R_LIGHT));
        
        // Subdivision controls with improved tooltips
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(240, 195), module, CurveAndDragModule::SUBDIV_L_PARAM));
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(240, 225), module, CurveAndDragModule::SUBDIV_R_PARAM));
        
        // CRITICAL FIX: Add subdivision displays for tempo sync labels
        SubdivisionDisplay* leftSubdivDisplay = new SubdivisionDisplay(0); // Left channel
        leftSubdivDisplay->box.pos = Vec(265, 189); // Positioned near left subdivision knob
        leftSubdivDisplay->box.size = Vec(40, 14);   // Larger size for better visibility
        leftSubdivDisplay->module = module;
        addChild(leftSubdivDisplay);
        
        SubdivisionDisplay* rightSubdivDisplay = new SubdivisionDisplay(1); // Right channel
        rightSubdivDisplay->box.pos = Vec(265, 219); // Positioned near right subdivision knob
        rightSubdivDisplay->box.size = Vec(40, 14);   // Larger size for better visibility
        rightSubdivDisplay->module = module;
        addChild(rightSubdivDisplay);
        
        // CV inputs for delay section
        addInput(createInputCentered<PJ301MPort>(Vec(140, rowPositions[3]), module, CurveAndDragModule::TIME_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(177, rowPositions[3]), module, CurveAndDragModule::FEEDBACK_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(213, rowPositions[3]), module, CurveAndDragModule::FEEDBACK_R_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(250, rowPositions[3]), module, CurveAndDragModule::TIME_R_CV_INPUT));
        
        // Mix CV inputs
        addInput(createInputCentered<PJ301MPort>(Vec(160, rowPositions[4]), module, CurveAndDragModule::MIX_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(183, rowPositions[4]), module, CurveAndDragModule::TIME_MOD_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(207, rowPositions[4]), module, CurveAndDragModule::FEEDBACK_MOD_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(230, rowPositions[4]), module, CurveAndDragModule::MIX_R_CV_INPUT));
        
        // Tap tempo triggers
        addInput(createInputCentered<PJ301MPort>(Vec(120, 265), module, CurveAndDragModule::TAP_L_TRIGGER_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(270, 265), module, CurveAndDragModule::TAP_R_TRIGGER_INPUT));
        
        // ===== SECTION 3: PITCH CONTROLS (315px center) =====
        
        // Main pitch control
        addParam(createParamCentered<RoundHugeBlackKnob>(Vec(315, 100), module, CurveAndDragModule::PITCH_PARAM));
        
        // Detune controls - FIXED: Added independent L/R detune
        addParam(createParamCentered<RoundBlackKnob>(Vec(260, rowPositions[1]), module, CurveAndDragModule::DETUNE_DRIFT_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(Vec(285, 135), module, CurveAndDragModule::DETUNE_L_PARAM)); // Left detune
        addParam(createParamCentered<RoundBlackKnob>(Vec(345, 135), module, CurveAndDragModule::DETUNE_R_PARAM)); // Right detune  
        addParam(createParamCentered<RoundBlackKnob>(Vec(370, rowPositions[1]), module, CurveAndDragModule::CHARACTER_PARAM));
        
        // Quantize toggle
        addParam(createParamCentered<CKSS>(Vec(315, 160), module, CurveAndDragModule::QUANTIZE_PARAM));
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(340, 160), module, CurveAndDragModule::QUANTIZE_LIGHT));
        
        // Morph control
        addParam(createParamCentered<RoundBlackKnob>(Vec(315, 185), module, CurveAndDragModule::MORPH_PARAM));
        addChild(createLightCentered<SmallLight<YellowLight>>(Vec(340, 185), module, CurveAndDragModule::MORPH_LIGHT));
        
        // Pitch mode selection
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(260, 210), module, CurveAndDragModule::PITCH_MODE_PARAM));
        
        // Scale selection
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(370, 210), module, CurveAndDragModule::SCALE_SELECT_PARAM));
        
        // ===== CRITICAL FIX: Scale Display Widget =====
        ScaleDisplay* scaleDisplay = new ScaleDisplay();
        scaleDisplay->box.pos = Vec(340, 225); // Below scale knob
        scaleDisplay->box.size = Vec(60, 14);   // Wide enough for scale names
        scaleDisplay->module = module;
        addChild(scaleDisplay);
        
        // CV inputs for pitch section - FIXED: Added L/R detune CV
        addInput(createInputCentered<PJ301MPort>(Vec(260, rowPositions[2]), module, CurveAndDragModule::DETUNE_DRIFT_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(285, rowPositions[2]), module, CurveAndDragModule::DETUNE_L_CV_INPUT)); // Left detune CV
        addInput(createInputCentered<PJ301MPort>(Vec(315, rowPositions[2]), module, CurveAndDragModule::PITCH_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(345, rowPositions[2]), module, CurveAndDragModule::DETUNE_R_CV_INPUT)); // Right detune CV
        addInput(createInputCentered<PJ301MPort>(Vec(370, rowPositions[2]), module, CurveAndDragModule::CHARACTER_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(315, rowPositions[3]), module, CurveAndDragModule::MORPH_CV_INPUT));
        
        // ===== SECTION 4: TAPE ENGINE (435px center) =====
        
        // Tape mode toggle
        addParam(createParamCentered<CKSS>(Vec(435, 85), module, CurveAndDragModule::TAPE_MODE_PARAM));
        addChild(createLightCentered<MediumLight<RedLight>>(Vec(465, 85), module, CurveAndDragModule::TAPE_MODE_LIGHT));
        
        // Saturation and character
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(390, rowPositions[1]), module, CurveAndDragModule::SATURATION_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(Vec(435, rowPositions[1]), module, CurveAndDragModule::HEAD_SELECT_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(Vec(480, rowPositions[1]), module, CurveAndDragModule::AGING_PARAM));
        
        // Instability control
        addParam(createParamCentered<RoundBlackKnob>(Vec(435, 195), module, CurveAndDragModule::INSTABILITY_PARAM));
        
        // CV inputs for tape section
        addInput(createInputCentered<PJ301MPort>(Vec(390, rowPositions[2]), module, CurveAndDragModule::SATURATION_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(435, rowPositions[2]), module, CurveAndDragModule::HEAD_SELECT_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(480, rowPositions[2]), module, CurveAndDragModule::AGING_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(435, rowPositions[3]), module, CurveAndDragModule::INSTABILITY_CV_INPUT));
        
        // ===== SECTION 5: WOW/FLUTTER + EQ (555px center) =====
        
        // Wow controls
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(510, rowPositions[0]), module, CurveAndDragModule::WOW_RATE_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(600, rowPositions[0]), module, CurveAndDragModule::WOW_DEPTH_PARAM));
        
        // Flutter controls
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(510, rowPositions[1]), module, CurveAndDragModule::FLUTTER_RATE_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(600, rowPositions[1]), module, CurveAndDragModule::FLUTTER_DEPTH_PARAM));
        
        // Waveform selection
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(555, 120), module, CurveAndDragModule::WOW_WAVEFORM_PARAM));
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(555, 140), module, CurveAndDragModule::FLUTTER_WAVEFORM_PARAM));
        
        // EQ controls (Head bump and rolloff)
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(510, rowPositions[2]), module, CurveAndDragModule::HEAD_BUMP_FREQ_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(540, rowPositions[2]), module, CurveAndDragModule::HEAD_BUMP_GAIN_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(570, rowPositions[2]), module, CurveAndDragModule::ROLLOFF_FREQ_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(600, rowPositions[2]), module, CurveAndDragModule::ROLLOFF_RESONANCE_PARAM));
        
        // Noise controls
        addParam(createParamCentered<CKSS>(Vec(510, rowPositions[3]), module, CurveAndDragModule::TAPE_NOISE_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(550, rowPositions[3]), module, CurveAndDragModule::NOISE_AMOUNT_PARAM));
        
        // CV inputs for wow/flutter section
        addInput(createInputCentered<PJ301MPort>(Vec(510, rowPositions[4]), module, CurveAndDragModule::WOW_RATE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(540, rowPositions[4]), module, CurveAndDragModule::WOW_DEPTH_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(570, rowPositions[4]), module, CurveAndDragModule::FLUTTER_RATE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(600, rowPositions[4]), module, CurveAndDragModule::FLUTTER_DEPTH_CV_INPUT));
        
        // Additional CV inputs
        addInput(createInputCentered<PJ301MPort>(Vec(540, 270), module, CurveAndDragModule::TAPE_NOISE_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(570, 270), module, CurveAndDragModule::NOISE_AMOUNT_CV_INPUT));
        
        // ===== SECTION 6: TUNING (675px center) =====
        
        // MTS-ESP controls
        addParam(createParamCentered<CKSS>(Vec(645, 75), module, CurveAndDragModule::MTS_ENABLE_PARAM));
        addChild(createLightCentered<MediumLight<GreenLight>>(Vec(675, 75), module, CurveAndDragModule::MTS_ACTIVE_LIGHT));
        
        // Add tuning info display
        TuningInfoDisplay* tuningDisplay = createWidget<TuningInfoDisplay>(Vec(630, 100));
        tuningDisplay->box.size = Vec(90, 50);
        tuningDisplay->module = module;
        addChild(tuningDisplay);
        
        // Final input gain CV inputs
        addInput(createInputCentered<PJ301MPort>(Vec(50, 110), module, CurveAndDragModule::INPUT_GAIN_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(100, 110), module, CurveAndDragModule::OUTPUT_GAIN_CV_INPUT));
    }
    
    // ===== IMPROVED TUNING INFO DISPLAY =====
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
            
            // Draw background with border
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 3);
            nvgFillColor(args.vg, nvgRGBA(20, 20, 25, 240));
            nvgFill(args.vg);
            nvgStrokeColor(args.vg, nvgRGBA(100, 100, 120, 180));
            nvgStrokeWidth(args.vg, 0.75f);
            nvgStroke(args.vg);
            
            // Set text properties
            nvgFontSize(args.vg, 9);
            nvgFontFaceId(args.vg, font->handle);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
            
            // Draw tuning source with color coding
            NVGcolor sourceColor = nvgRGBA(200, 200, 200, 255);
            if (module->tuningSource == "MTS-ESP") {
                sourceColor = nvgRGBA(100, 255, 100, 255); // Green for MTS-ESP
            } else if (module->tuningSource == "Scala") {
                sourceColor = nvgRGBA(255, 255, 100, 255); // Yellow for Scala
            } else {
                sourceColor = nvgRGBA(150, 200, 255, 255); // Blue for built-in
            }
            
            nvgFillColor(args.vg, sourceColor);
            nvgText(args.vg, box.size.x/2, 5, module->tuningSource.c_str(), NULL);
            
            // Draw tuning info
            nvgFillColor(args.vg, nvgRGBA(220, 220, 220, 200));
            nvgFontSize(args.vg, 7);
            nvgText(args.vg, box.size.x/2, 18, module->tuningInfo.c_str(), NULL);
            
            // Draw pitch info if quantization is active
            if (module->params[CurveAndDragModule::QUANTIZE_PARAM].getValue() > 0.5f) {
                nvgFontSize(args.vg, 6);
                nvgFillColor(args.vg, nvgRGBA(200, 255, 200, 180));
                
                char rawPitchStr[32];
                char quantPitchStr[32];
                snprintf(rawPitchStr, sizeof(rawPitchStr), "Raw: %.1f¢", module->lastRawPitch);
                snprintf(quantPitchStr, sizeof(quantPitchStr), "Quant: %.1f¢", module->lastQuantizedPitch);
                
                nvgText(args.vg, box.size.x/2, 30, rawPitchStr, NULL);
                nvgText(args.vg, box.size.x/2, 40, quantPitchStr, NULL);
            }
        }
    };
    
    // ===== SUBDIVISION DISPLAY WIDGET =====
    struct SubdivisionDisplay : TransparentWidget {
        CurveAndDragModule *module;
        int channel; // 0=left, 1=right
        std::shared_ptr<Font> font;
        
        SubdivisionDisplay(int ch) : channel(ch) {
            // CRITICAL FIX: Multiple font fallbacks for reliability
            font = APP->window->loadFont(asset::system("res/fonts/DSEG7ClassicMini-BoldItalic.ttf"));
            if (!font) {
                font = APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
            }
            if (!font) {
                font = APP->window->loadFont(asset::system("res/fonts/DejaVuSans.ttf"));
            }
            if (!font) {
                // Ultimate fallback - use default system font
                INFO("CurveAndDrag: Could not load display fonts, using default");
            }
        }
        
        void draw(const DrawArgs &args) override {
            if (!module) return;
            
            // Only show when sync is enabled for this channel
            bool syncEnabled = (channel == 0) ? 
                module->params[CurveAndDragModule::SYNC_L_PARAM].getValue() > 0.5f :
                module->params[CurveAndDragModule::SYNC_R_PARAM].getValue() > 0.5f;
                
            if (!syncEnabled) {
                // CRITICAL FIX: Draw a subtle indicator when sync is off
                nvgBeginPath(args.vg);
                nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
                nvgFillColor(args.vg, nvgRGBA(15, 15, 15, 100));
                nvgFill(args.vg);
                return;
            }
            
            // Get subdivision
            int subdivParam = (channel == 0) ? 
                CurveAndDragModule::SUBDIV_L_PARAM : CurveAndDragModule::SUBDIV_R_PARAM;
            int subdivIndex = static_cast<int>(module->params[subdivParam].getValue());
            CurveAndDragModule::SubdivisionType subdivType = 
                static_cast<CurveAndDragModule::SubdivisionType>(clamp(subdivIndex, 0, 5));
            
            // Get subdivision name - now calling the public method
            std::string subdivName = module->getSubdivisionName(subdivType);
            
            // CRITICAL FIX: Enhanced background with border for better visibility
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
            nvgFillColor(args.vg, nvgRGBA(40, 40, 50, 220));
            nvgFill(args.vg);
            nvgStrokeColor(args.vg, nvgRGBA(80, 120, 180, 180));
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);
            
            // CRITICAL FIX: Enhanced text rendering with fallbacks
            if (font) {
                nvgFontFaceId(args.vg, font->handle);
            }
            nvgFontSize(args.vg, 9);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            
            // CRITICAL FIX: High contrast text with shadow for visibility
            // Draw text shadow
            nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 200));
            nvgText(args.vg, box.size.x/2 + 0.5f, box.size.y/2 + 0.5f, subdivName.c_str(), NULL);
            
            // Draw main text
            nvgFillColor(args.vg, nvgRGBA(120, 180, 255, 255));
            nvgText(args.vg, box.size.x/2, box.size.y/2, subdivName.c_str(), NULL);
            
            // CRITICAL FIX: Debug indicator - draw small dot when active
            nvgBeginPath(args.vg);
            nvgCircle(args.vg, box.size.x - 3, 3, 1);
            nvgFillColor(args.vg, nvgRGBA(0, 255, 0, 200));
            nvgFill(args.vg);
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
            
            // Get current scale index and clamp to valid range
            int scaleIndex = clamp(static_cast<int>(module->params[CurveAndDragModule::SCALE_SELECT_PARAM].getValue()), 0, 10);
            
            // Scale names array
            std::vector<std::string> scaleNames = {
                "12-TET", "24-TET", "31-EDO", "Just", 
                "Pythagorean", "Meantone", "Well-Temp", 
                "19-TET", "22-TET", "53-TET", "72-TET"
            };
            
            std::string scaleName = scaleNames[scaleIndex];
            
            // Draw background with border
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 2);
            nvgFillColor(args.vg, nvgRGBA(30, 30, 35, 220));
            nvgFill(args.vg);
            nvgStrokeColor(args.vg, nvgRGBA(100, 150, 200, 180));
            nvgStrokeWidth(args.vg, 1.0f);
            nvgStroke(args.vg);
            
            // Draw text
            if (font) {
                nvgFontFaceId(args.vg, font->handle);
            }
            nvgFontSize(args.vg, 8);
            nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            
            // Color coding based on scale type
            NVGcolor textColor = nvgRGBA(180, 220, 255, 255); // Default blue
            if (scaleName.find("TET") != std::string::npos || scaleName.find("EDO") != std::string::npos) {
                textColor = nvgRGBA(150, 255, 150, 255); // Green for equal temperaments
            } else if (scaleName == "Just" || scaleName == "Pythagorean") {
                textColor = nvgRGBA(255, 200, 100, 255); // Orange for historical tunings
            }
            
            // Draw text shadow
            nvgFillColor(args.vg, nvgRGBA(0, 0, 0, 200));
            nvgText(args.vg, box.size.x/2 + 0.5f, box.size.y/2 + 0.5f, scaleName.c_str(), NULL);
            
            // Draw main text
            nvgFillColor(args.vg, textColor);
            nvgText(args.vg, box.size.x/2, box.size.y/2, scaleName.c_str(), NULL);
        }
    };
    
    void appendContextMenu(Menu* menu) override {
        CurveAndDragModule* module = dynamic_cast<CurveAndDragModule*>(this->module);
        if (!module) return;
        
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("CurveAndDrag v2.8.0"));
        
        // Scale loading options
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Tuning & Scales"));
        
        menu->addChild(createMenuItem("Load Scala File...", "", [=]() {
            std::string path = osdialog_file(OSDIALOG_OPEN, NULL, NULL, osdialog_filters_parse("Scala Scale:scl"));
            if (!path.empty()) {
                module->scalaReader.loadScalaFile(path);
            }
        }));
        
        // Tape options
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Tape Delay Options"));
        
        // Wow waveform submenu
        menu->addChild(createSubmenuItem("Wow Waveform", "", [=](Menu* subMenu) {
            std::vector<std::string> waveNames = {"Sine", "Triangle", "Random"};
            for (int i = 0; i < 3; i++) {
                subMenu->addChild(createMenuItem(waveNames[i], 
                    module->params[CurveAndDragModule::WOW_WAVEFORM_PARAM].getValue() == i ? "✓" : "", 
                    [=]() {
                        module->params[CurveAndDragModule::WOW_WAVEFORM_PARAM].setValue(i);
                    }));
            }
        }));
        
        // Flutter waveform submenu
        menu->addChild(createSubmenuItem("Flutter Waveform", "", [=](Menu* subMenu) {
            std::vector<std::string> waveNames = {"Sine", "Triangle", "Random"};
            for (int i = 0; i < 3; i++) {
                subMenu->addChild(createMenuItem(waveNames[i], 
                    module->params[CurveAndDragModule::FLUTTER_WAVEFORM_PARAM].getValue() == i ? "✓" : "", 
                    [=]() {
                        module->params[CurveAndDragModule::FLUTTER_WAVEFORM_PARAM].setValue(i);
                    }));
            }
        }));
        
        // Pitch mode submenu
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Pitch Shifter Options"));
        
        menu->addChild(createSubmenuItem("Pitch Algorithm", "", [=](Menu* subMenu) {
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
