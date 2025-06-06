#pragma once
#include "CurveAndDrag.hpp"
#include <rack.hpp>

namespace CurveAndDrag {

struct CurveAndDragWidget : ModuleWidget {
    CurveAndDragWidget(CurveAndDragModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/CurveAndDrag.svg")));

        // Panel dimensions (assuming standard sizing)
        const float width = box.size.x;
        const float height = box.size.y;
        
        // Define spacing and positioning constants
        const float margin = 5.0f;
        const float leftX = margin;
        const float rightX = width - margin * 6;  // For right channel controls
        const float centerX = width / 2.0f;
        
        const float rowHeight = 30.0f;
        float currentY = margin * 3;
        
        // MTS-ESP indicator light at the top
        addChild(createLightCentered<MediumLight<BlueLight>>(Vec(centerX - 30, currentY - 2), module, CurveAndDragModule::MTS_ACTIVE_LIGHT));
        addParam(createParamCentered<CKSS>(Vec(centerX - 30, currentY + 12), module, CurveAndDragModule::MTS_ENABLE_PARAM));

        // Add left-channel delay time control
        addParam(createParamCentered<RoundBlackKnob>(Vec(leftX + 20, currentY + 15), module, CurveAndDragModule::TIME_L_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(leftX + 50, currentY + 15), module, CurveAndDragModule::TIME_L_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(leftX + 80, currentY + 15), module, CurveAndDragModule::TAP_L_TRIGGER_INPUT));
        
        // Add right-channel delay time control
        addParam(createParamCentered<RoundBlackKnob>(Vec(rightX + 20, currentY + 15), module, CurveAndDragModule::TIME_R_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(rightX + 50, currentY + 15), module, CurveAndDragModule::TIME_R_CV_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(rightX + 80, currentY + 15), module, CurveAndDragModule::TAP_R_TRIGGER_INPUT));
        
        currentY += rowHeight * 1.5;
        
        // Add sync toggles
        addParam(createParamCentered<CKSS>(Vec(leftX + 20, currentY + 10), module, CurveAndDragModule::SYNC_L_PARAM));
        addChild(createLightCentered<MediumLight<GreenLight>>(Vec(leftX + 20, currentY + 25), module, CurveAndDragModule::SYNC_L_LIGHT));
        
        addParam(createParamCentered<CKSS>(Vec(rightX + 20, currentY + 10), module, CurveAndDragModule::SYNC_R_PARAM));
        addChild(createLightCentered<MediumLight<GreenLight>>(Vec(rightX + 20, currentY + 25), module, CurveAndDragModule::SYNC_R_LIGHT));
        
        currentY += rowHeight;
        
        // Add left-channel feedback control
        addParam(createParamCentered<RoundBlackKnob>(Vec(leftX + 20, currentY + 15), module, CurveAndDragModule::FEEDBACK_L_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(leftX + 50, currentY + 15), module, CurveAndDragModule::FEEDBACK_L_CV_INPUT));
        
        // Add right-channel feedback control
        addParam(createParamCentered<RoundBlackKnob>(Vec(rightX + 20, currentY + 15), module, CurveAndDragModule::FEEDBACK_R_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(rightX + 50, currentY + 15), module, CurveAndDragModule::FEEDBACK_R_CV_INPUT));
        
        currentY += rowHeight;
        
        // Add cross-feedback toggle
        addParam(createParamCentered<CKSS>(Vec(centerX, currentY + 10), module, CurveAndDragModule::CROSS_FEEDBACK_PARAM));
        addChild(createLightCentered<MediumLight<GreenLight>>(Vec(centerX, currentY + 25), module, CurveAndDragModule::CROSS_FEEDBACK_LIGHT));
        
        currentY += rowHeight;
        
        // Add left-channel mix control
        addParam(createParamCentered<RoundBlackKnob>(Vec(leftX + 20, currentY + 15), module, CurveAndDragModule::MIX_L_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(leftX + 50, currentY + 15), module, CurveAndDragModule::MIX_L_CV_INPUT));
        
        // Add right-channel mix control
        addParam(createParamCentered<RoundBlackKnob>(Vec(rightX + 20, currentY + 15), module, CurveAndDragModule::MIX_R_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(rightX + 50, currentY + 15), module, CurveAndDragModule::MIX_R_CV_INPUT));
        
        currentY += rowHeight * 1.5;
        
        // Add pitch shift control
        addParam(createParamCentered<RoundBlackKnob>(Vec(centerX - 25, currentY + 15), module, CurveAndDragModule::PITCH_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 25, currentY + 15), module, CurveAndDragModule::PITCH_CV_INPUT));
        
        currentY += rowHeight;
        
        // Add detune drift control
        addParam(createParamCentered<RoundBlackKnob>(Vec(leftX + 20, currentY + 15), module, CurveAndDragModule::DETUNE_DRIFT_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(leftX + 50, currentY + 15), module, CurveAndDragModule::DETUNE_DRIFT_CV_INPUT));
        
        // Add quantize toggle
        addParam(createParamCentered<CKSS>(Vec(centerX, currentY + 15), module, CurveAndDragModule::QUANTIZE_PARAM));
        addChild(createLightCentered<MediumLight<GreenLight>>(Vec(centerX, currentY + 30), module, CurveAndDragModule::QUANTIZE_LIGHT));
        
        // Add scale selection
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(rightX + 20, currentY + 15), module, CurveAndDragModule::SCALE_SELECT_PARAM));
        
        currentY += rowHeight;
        
        // Add morph control
        addParam(createParamCentered<RoundBlackKnob>(Vec(centerX - 25, currentY + 15), module, CurveAndDragModule::MORPH_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 25, currentY + 15), module, CurveAndDragModule::MORPH_CV_INPUT));
        
        currentY += rowHeight * 1.25;
        
        // Tape delay section label
        addChild(createWidget<Widget>(Vec(centerX - 35, currentY))); // Placeholder for label
        
        // Tape mode toggle
        addParam(createParamCentered<CKSS>(Vec(leftX + 20, currentY + 15), module, CurveAndDragModule::TAPE_MODE_PARAM));
        addChild(createLightCentered<MediumLight<RedLight>>(Vec(leftX + 20, currentY + 30), module, CurveAndDragModule::TAPE_MODE_LIGHT));
        
        currentY += rowHeight;
        
        // Wow controls
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(leftX + 20, currentY + 15), module, CurveAndDragModule::WOW_RATE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(leftX + 50, currentY + 15), module, CurveAndDragModule::WOW_RATE_CV_INPUT));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(leftX + 80, currentY + 15), module, CurveAndDragModule::WOW_DEPTH_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(leftX + 110, currentY + 15), module, CurveAndDragModule::WOW_DEPTH_CV_INPUT));
        
        // Wow waveform selection
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(rightX + 20, currentY + 15), module, CurveAndDragModule::WOW_WAVEFORM_PARAM));
        
        currentY += rowHeight;
        
        // Flutter controls
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(leftX + 20, currentY + 15), module, CurveAndDragModule::FLUTTER_RATE_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(leftX + 50, currentY + 15), module, CurveAndDragModule::FLUTTER_RATE_CV_INPUT));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(leftX + 80, currentY + 15), module, CurveAndDragModule::FLUTTER_DEPTH_PARAM));
        
        // Flutter waveform selection
        addParam(createParamCentered<RoundBlackSnapKnob>(Vec(rightX + 20, currentY + 15), module, CurveAndDragModule::FLUTTER_WAVEFORM_PARAM));
        
        currentY += rowHeight;
        
        // Saturation control
        addParam(createParamCentered<RoundBlackKnob>(Vec(centerX - 25, currentY + 15), module, CurveAndDragModule::SATURATION_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(centerX + 25, currentY + 15), module, CurveAndDragModule::SATURATION_CV_INPUT));
        
        currentY += rowHeight;
        
        // Head bump & EQ controls
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(leftX + 20, currentY + 15), module, CurveAndDragModule::HEAD_BUMP_FREQ_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(leftX + 50, currentY + 15), module, CurveAndDragModule::HEAD_BUMP_GAIN_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(rightX + 20, currentY + 15), module, CurveAndDragModule::ROLLOFF_FREQ_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(rightX + 50, currentY + 15), module, CurveAndDragModule::ROLLOFF_RESONANCE_PARAM));
        
        currentY += rowHeight;
        
        // Noise controls
        addParam(createParamCentered<CKSS>(Vec(leftX + 20, currentY + 15), module, CurveAndDragModule::TAPE_NOISE_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(Vec(leftX + 50, currentY + 15), module, CurveAndDragModule::NOISE_AMOUNT_PARAM));
        addInput(createInputCentered<PJ301MPort>(Vec(leftX + 80, currentY + 15), module, CurveAndDragModule::NOISE_AMOUNT_CV_INPUT));
        
        currentY += rowHeight * 1.5;
        
        // Add level meters
        currentY += rowHeight * 1.25;
        
        // Left channel level meter
        for (int i = 0; i < 5; i++) {
            addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(leftX + 20 + i * 8, currentY + 10), module, CurveAndDragModule::LEVEL_LIGHTS_L_START + i));
        }
        
        // Right channel level meter
        for (int i = 0; i < 5; i++) {
            addChild(createLightCentered<SmallLight<GreenRedLight>>(Vec(rightX + 20 + i * 8, currentY + 10), module, CurveAndDragModule::LEVEL_LIGHTS_R_START + i));
        }
        
        // Add input/output ports at bottom
        const float portsY = height - margin * 5;
        
        // Input ports
        addInput(createInputCentered<PJ301MPort>(Vec(leftX + 20, portsY), module, CurveAndDragModule::LEFT_INPUT));
        addInput(createInputCentered<PJ301MPort>(Vec(rightX + 20, portsY), module, CurveAndDragModule::RIGHT_INPUT));
        
        // Output ports
        addOutput(createOutputCentered<PJ301MPort>(Vec(leftX + 60, portsY), module, CurveAndDragModule::LEFT_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(Vec(rightX + 60, portsY), module, CurveAndDragModule::RIGHT_OUTPUT));
    }
    
    // Custom display for tuning information
    struct TuningInfoDisplay : TransparentWidget {
        CurveAndDragModule *module;
        std::shared_ptr<Font> font;
        
        TuningInfoDisplay() {
            font = APP->window->loadFont(asset::plugin(pluginInstance, "res/fonts/Roboto-Regular.ttf"));
        }
        
        void draw(const DrawArgs &args) override {
            if (!module) return;
            
            // Draw background
            nvgBeginPath(args.vg);
            nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 4);
            nvgFillColor(args.vg, nvgRGBA(40, 40, 40, 200));
            nvgFill(args.vg);
            
            // Set text properties
            nvgFontSize(args.vg, 12);
            nvgFontFaceId(args.vg, font->handle);
            nvgTextAlign(args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
            
            // Draw tuning source
            nvgFillColor(args.vg, nvgRGBA(0xFF, 0xFF, 0xFF, 0xE0));
            nvgText(args.vg, 5, 5, module->tuningSource.c_str(), NULL);
            
            // Draw tuning info (if available)
            if (!module->tuningInfo.empty()) {
                nvgText(args.vg, 5, 20, module->tuningInfo.c_str(), NULL);
            }
        }
    };
    
    void appendContextMenu(Menu* menu) override {
        CurveAndDragModule* module = dynamic_cast<CurveAndDragModule*>(this->module);
        
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Scale Options"));
        
        // Add menu items for loading Scala files
        menu->addChild(createMenuItem("Load Scala File...", "", [=]() {
            // This would open a file dialog for loading .scl files
            // Implementation would depend on VCV file dialog API
        }));
        
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("Tape Delay Options"));
        
        // Add wow waveform selection submenu
        menu->addChild(createSubmenuItem("Wow Waveform", "", [=](Menu* subMenu) {
            subMenu->addChild(createMenuItem("Sine", module->params[CurveAndDragModule::WOW_WAVEFORM_PARAM].getValue() == 0 ? "✓" : "", [=]() {
                module->params[CurveAndDragModule::WOW_WAVEFORM_PARAM].setValue(0);
            }));
            subMenu->addChild(createMenuItem("Triangle", module->params[CurveAndDragModule::WOW_WAVEFORM_PARAM].getValue() == 1 ? "✓" : "", [=]() {
                module->params[CurveAndDragModule::WOW_WAVEFORM_PARAM].setValue(1);
            }));
            subMenu->addChild(createMenuItem("Random", module->params[CurveAndDragModule::WOW_WAVEFORM_PARAM].getValue() == 2 ? "✓" : "", [=]() {
                module->params[CurveAndDragModule::WOW_WAVEFORM_PARAM].setValue(2);
            }));
        }));
        
        // Add flutter waveform selection submenu
        menu->addChild(createSubmenuItem("Flutter Waveform", "", [=](Menu* subMenu) {
            subMenu->addChild(createMenuItem("Sine", module->params[CurveAndDragModule::FLUTTER_WAVEFORM_PARAM].getValue() == 0 ? "✓" : "", [=]() {
                module->params[CurveAndDragModule::FLUTTER_WAVEFORM_PARAM].setValue(0);
            }));
            subMenu->addChild(createMenuItem("Triangle", module->params[CurveAndDragModule::FLUTTER_WAVEFORM_PARAM].getValue() == 1 ? "✓" : "", [=]() {
                module->params[CurveAndDragModule::FLUTTER_WAVEFORM_PARAM].setValue(1);
            }));
            subMenu->addChild(createMenuItem("Random", module->params[CurveAndDragModule::FLUTTER_WAVEFORM_PARAM].getValue() == 2 ? "✓" : "", [=]() {
                module->params[CurveAndDragModule::FLUTTER_WAVEFORM_PARAM].setValue(2);
            }));
        }));
        
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("MTS-ESP Microtuning"));
        
        // Add MTS-ESP toggle
        menu->addChild(createBoolMenuItem("Enable MTS-ESP", "", [=]() {
            return module->params[CurveAndDragModule::MTS_ENABLE_PARAM].getValue() > 0.5f;
        }, [=](bool val) {
            module->params[CurveAndDragModule::MTS_ENABLE_PARAM].setValue(val ? 1.f : 0.f);
        }));
    }
};

} // namespace CurveAndDrag
