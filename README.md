# CurveAndDrag

📘 **Overview**

CurveAndDrag is a professional stereo delay and pitch shifter module for VCV Rack featuring analog-inspired DSP, comprehensive tape delay modeling, and advanced microtonal tuning support. Combining four distinct pitch shifting algorithms with multi-head tape delay emulation, cross-channel feedback, and extensive CV automation, CurveAndDrag delivers studio-grade delay effects with vintage character and modern precision.

## 🚀 Key Features

- **Four Pitch Shifting Algorithms**: BBD (Bucket Brigade), H910 (Harmonizer), Varispeed (Tape), and Hybrid modes
- **Multi-Head Tape Delay**: 1-4 configurable tape heads with authentic wow/flutter modeling
- **Independent Stereo Processing**: Separate left/right delay times, feedback, and detune controls
- **Comprehensive Tape DSP**: Saturation, aging, instability, noise (0-8%), and head EQ
- **Advanced Quantization**: 11 built-in scales plus MTS-ESP and Scala file support
- **Tempo Sync**: Musical subdivisions (1/1, 1/2, 1/4, 1/8, 1/8T, 1/16) with tap tempo
- **Cross-Channel Feedback**: Stereo enhancement with progressive limiting
- **Extensive CV Control**: 21 CV inputs for complete automation
- **Real-Time Displays**: Subdivision, scale, tuning info, and 5-segment level meters
- **Professional Layout**: 48HP module with logical 6-section organization

## 🎛 Parameters and Controls

### Core Delay Parameters
| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| Time L | Knob | 0-2000ms | 200ms | Left channel delay time |
| Time R | Knob | 0-2000ms | 200ms | Right channel delay time |
| Feedback L | Knob | 0-110% | 30% | Left channel feedback amount |
| Feedback R | Knob | 0-110% | 30% | Right channel feedback amount |
| Mix L | Knob | 0-100% | 50% | Left channel dry/wet mix |
| Mix R | Knob | 0-100% | 50% | Right channel dry/wet mix |
| Cross-Feedback | Knob | 0-100% | 0% | Cross-channel feedback amount |

### Tempo Sync & Subdivision
| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| Sync L | Toggle | On/Off | Off | Left channel tempo sync enable |
| Sync R | Toggle | On/Off | Off | Right channel tempo sync enable |
| Subdiv L | Selector | 0-5 | 2 | Left subdivision (1/1, 1/2, 1/4, 1/8, 1/8T, 1/16) |
| Subdiv R | Selector | 0-5 | 2 | Right subdivision (1/1, 1/2, 1/4, 1/8, 1/8T, 1/16) |

### Pitch Shifting System
| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| Main Pitch | Knob | ±2 octaves | 0 | Global pitch shift in cents |
| Detune L | Knob | ±50 cents | 0 | Left channel fine detune |
| Detune R | Knob | ±50 cents | 0 | Right channel fine detune |
| Detune Drift | Knob | 0-25 cents | 0 | Stereo detune movement |
| Pitch Mode | Selector | 0-3 | 1 | Algorithm: BBD/H910/Varispeed/Hybrid |
| Character | Knob | 0-100% | 50% | Vintage pitch shifter modeling |
| Morph | Knob | 0-100% | 0% | Blend between pitch algorithms |

### Scale & Tuning
| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| Quantize | Toggle | On/Off | Off | Enable pitch quantization |
| Scale Select | Selector | 0-10 | 0 | Built-in scale selection |
| MTS-ESP | Toggle | On/Off | Off | External tuning source enable |

**Built-in Scales**: 12-TET, 24-TET, 31-EDO, Just Intonation, Pythagorean, Meantone, Well-Tempered, 19-TET, 22-TET, 53-TET, 72-TET

### Tape DSP Engine
| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| Tape Mode | Toggle | On/Off | Off | Enable tape delay processing |
| Saturation | Knob | 0-100% | 30% | Tape saturation amount |
| Aging | Knob | 0-100% | 0% | Vintage wear and frequency response |
| Instability | Knob | 0-100% | 0% | Speed variations and dropouts |
| Noise Enable | Toggle | On/Off | Off | Tape noise enable |
| Noise Amount | Knob | 0-8% | 0.8% | Tape noise level |
| Head Config | Selector | 1-4 | 1 | Number of active tape heads |

### Modulation (Wow & Flutter)
| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| Wow Rate | Knob | 0.1-5.0 Hz | 0.3 Hz | Wow oscillation rate |
| Wow Depth | Knob | 0-100% | 20% | Wow modulation depth |
| Flutter Rate | Knob | 1.0-10.0 Hz | 2.7 Hz | Flutter oscillation rate |
| Flutter Depth | Knob | 0-100% | 10% | Flutter modulation depth |
| Wow Waveform | Selector | 0-2 | 0 | Sine/Triangle/Random |
| Flutter Waveform | Selector | 0-2 | 0 | Sine/Triangle/Random |

### Head EQ System
| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| Head Bump Freq | Knob | 60-120 Hz | 90 Hz | Low frequency resonance |
| Head Bump Gain | Knob | 0-3x | 1.2x | Resonance gain |
| Rolloff Freq | Knob | 3-15 kHz | 10 kHz | High frequency rolloff |
| Rolloff Resonance | Knob | 0.5-2.0 | 0.7 | Rolloff Q factor |

### Gain Controls
| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| Input Gain | Knob | 0-2x | 1x | Input signal gain |
| Output Gain | Knob | 0-2x | 1x | Output signal gain |

## 📶 CV Inputs

| CV Input | Target Parameter | Range | Description |
|----------|------------------|-------|-------------|
| Input Gain CV | Input Gain | ±10V | Input level modulation |
| Output Gain CV | Output Gain | ±10V | Output level modulation |
| Time L CV | Left Time | ±10V | Left delay time modulation |
| Time R CV | Right Time | ±10V | Right delay time modulation |
| Feedback L CV | Left Feedback | ±10V | Left feedback modulation |
| Feedback R CV | Right Feedback | ±10V | Right feedback modulation |
| Mix L CV | Left Mix | ±10V | Left mix modulation |
| Mix R CV | Right Mix | ±10V | Right mix modulation |
| Time Mod CV | Global Time | ±10V | Global time modulation (±100ms) |
| Feedback Mod CV | Global Feedback | ±10V | Global feedback modulation (±10%) |
| Pitch CV | Main Pitch | ±10V | Main pitch modulation (1V/oct scaling) |
| Detune L CV | Left Detune | ±10V | Left detune modulation |
| Detune R CV | Right Detune | ±10V | Right detune modulation |
| Detune Drift CV | Drift | ±10V | Stereo drift modulation |
| Morph CV | Algorithm Morph | ±10V | Pitch algorithm blending |
| Character CV | Character | ±10V | Vintage modeling amount |
| Wow Rate CV | Wow Rate | ±10V | Wow rate modulation |
| Wow Depth CV | Wow Depth | ±10V | Wow depth modulation |
| Flutter Rate CV | Flutter Rate | ±10V | Flutter rate modulation |
| Flutter Depth CV | Flutter Depth | ±10V | Flutter depth modulation |
| Saturation CV | Saturation | ±10V | Tape saturation modulation |
| Aging CV | Aging | ±10V | Tape aging modulation |
| Instability CV | Instability | ±10V | Tape instability modulation |
| Tape Noise CV | Noise Enable | ±10V | Noise enable trigger |
| Noise Amount CV | Noise Amount | ±10V | Noise level modulation |
| Head Select CV | Head Config | ±10V | Head configuration selection |

## 🔄 Signal Flow

```
Audio Input → Input Gain → Pitch Shifting → Delay Lines → Cross-Feedback → Tape Processing → Output Mix → Audio Output
     ↓              ↓            ↓              ↓              ↓              ↓              ↓
   Gain CV     Character CV   Pitch CVs     Delay CVs    Cross-FB Param   Tape CVs     Mix/Gain CVs
```

**Detailed Flow:**
1. **Input Stage**: Audio input with gain control and CV modulation
2. **Pitch Processing**: Four algorithms (BBD/H910/Varispeed/Hybrid) with quantization
3. **Delay Processing**: Independent L/R delay lines with tempo sync capability
4. **Cross-Feedback**: Filtered cross-channel feedback with progressive limiting
5. **Tape Emulation**: Comprehensive tape modeling (wow/flutter, saturation, aging, noise)
6. **Output Stage**: Dry/wet mixing with independent wet outputs and level metering

**Key Signal Path Features:**
- Independent stereo processing throughout entire chain
- Cross-feedback applied after delay but before tape processing
- Pitch shifting with automatic gain compensation
- Tape effects as final processing stage before output mixing
- Real-time level metering with 5-segment displays

## 📦 Installation

### Automatic Installation
1. Download `CurveAndDrag-1.0-WINDOWS-x64.vcvplugin` from releases
2. Double-click to install in VCV Rack

### Manual Installation (Windows)
1. Extract plugin contents to individual files
2. Copy to `%APPDATA%/Rack2/plugins-win-x64/CurveAndDrag/`:
   - `plugin.dll`
   - `plugin.json`
   - `res/CurveAndDrag.svg`

### Dependencies
- VCV Rack 2.0 or higher
- Windows x64 (tested on Windows 10/11)
- Optional: MTS-ESP host for external tuning support

## ⚠️ Known Issues & Tips

### Performance Notes
- **Large Pitch Shifts**: Shifts >±1 octave may introduce latency artifacts
- **High Feedback**: Feedback >90% requires careful gain staging
- **Tape Instability**: High instability values can cause temporary dropouts (by design)

### Usage Tips
- **Tempo Sync**: Use tap tempo inputs for manual BPM detection
- **Cross-Feedback**: Start with low values (10-20%) to avoid resonance
- **Tape Noise**: 0-8% range provides authentic vintage character without masking signal
- **Head Configuration**: Multiple heads create rhythmic echoes with slight timing offsets
- **Scale Quantization**: Combine with external sequencers for microtonal composition

### Limitations
- **MTS-ESP**: Requires separate MTS-ESP host plugin for external tuning
- **Scala Files**: Load via right-click menu, limited to 12-tone octave patterns
- **Pitch Tracking**: Optimized for tonal material, may not track percussion accurately

## 🧪 Testing & Validation

### Verified Features
✅ All 37 parameters respond correctly to manual control  
✅ All 21 CV inputs provide proper modulation ranges  
✅ Tempo sync works with all 6 subdivision types  
✅ All 4 pitch algorithms function with gain compensation  
✅ Tape processing includes complete DSP chain  
✅ Built-in scales quantize accurately  
✅ Level meters respond to both input and output signals  
✅ Cross-feedback stable at all settings  

### Test Patch Ideas
- **Rhythmic Delays**: Sync different subdivisions to create polyrhythmic patterns
- **Pitch Harmonization**: Use detune L/R with quantization for harmonic intervals
- **Tape Character**: Combine aging, saturation, and noise for authentic vintage sound
- **Modulated Delays**: Use wow/flutter with high depths for experimental textures
- **Microtonal Sequences**: Connect CV sequencer to scale selector for dynamic tuning changes

## 📖 Licensing & Credits

**License**: MIT License - see LICENSE file for details

**Author**: Joshua McDonagh  
**Email**: josh.mcd31@gmail.com  
**GitHub**: https://github.com/shua-ie/curveanddrag

**Special Thanks**: 
- VCV Rack community for DSP optimization techniques
- MTS-ESP specification contributors for microtonal support
- Vintage tape delay manufacturers for authentic modeling references

## 📈 Version History

**v1.0** (Current) - Initial Professional Release
- Complete 48HP professional layout with 6-section organization
- All 37 parameters fully functional with CV automation
- Enhanced tape DSP with proper 0-8% noise scaling
- Independent L/R pitch processing with gain compensation
- Real-time displays for subdivisions, scales, and tuning info
- Cross-feedback system with progressive limiting
- Professional parameter tooltips and status lighting
- Four pitch shifting algorithms (BBD/H910/Varispeed/Hybrid)
- Complete tape delay emulation with wow/flutter modeling
- 11 built-in scales plus MTS-ESP and Scala file support
- 21 CV inputs for comprehensive automation
- Tempo sync with musical subdivisions and tap tempo

---

*CurveAndDrag v1.0 - Professional Stereo Delay & Pitch Shifter for VCV Rack*
