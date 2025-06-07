# Changelog

All notable changes to CurveAndDrag will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2025-01-07 - Initial Professional Release

### 🎉 Added - Complete Feature Set

#### Professional Delay & Pitch System
- **Four Pitch Shifting Algorithms**: BBD (Bucket Brigade), H910 (Harmonizer), Varispeed (Tape), and Hybrid modes
- **Multi-Head Tape Delay**: 1-4 configurable tape heads with authentic wow/flutter modeling
- **Independent Stereo Processing**: Separate left/right delay times, feedback, and detune controls
- **Comprehensive Tape DSP**: Saturation, aging, instability, noise (0-8%), and head EQ
- **Cross-Channel Feedback**: Stereo enhancement with progressive limiting

#### Advanced Microtonal Support
- **MTS-ESP Integration**: Real-time tuning table updates from external sources
- **11 Built-in Scales**: 12-TET, 24-TET, 31-EDO, Just Intonation, Pythagorean, Meantone, Well-Tempered, 19-TET, 22-TET, 53-TET, 72-TET
- **Scala File Support**: Load .scl format files for custom temperaments
- **Advanced Quantization**: Priority system (MTS-ESP → Scala → Built-in scales)

#### Professional Control System
- **37 Parameters**: Complete control over all delay, pitch, and tape parameters
- **21 CV Inputs**: Full automation capability for all major parameters
- **Tempo Sync**: Musical subdivisions (1/1, 1/2, 1/4, 1/8, 1/8T, 1/16) with tap tempo
- **Real-Time Displays**: Subdivision, scale, tuning info, and 5-segment level meters

#### User Interface
- **Professional 48HP Layout**: Logical 6-section organization (Input+Sync, Delay Engine, Pitch System, Tape DSP, Modulation, Output)
- **Status Indicators**: LEDs for tempo sync, tape mode, MTS connection, and activity
- **Parameter Displays**: Real-time feedback for subdivisions, scales, and tuning information
- **Visual Feedback**: Clear status lighting for all switch states and connections

### 🎛 Parameter Groups Implemented

#### Core Delay Parameters (7 parameters)
- Time L/R, Feedback L/R, Mix L/R, Cross-Feedback

#### Tempo Sync & Subdivision (4 parameters)  
- Sync L/R toggles, Subdivision L/R selectors

#### Pitch Shifting System (7 parameters)
- Main Pitch, Detune L/R, Detune Drift, Pitch Mode, Character, Morph

#### Scale & Tuning (3 parameters)
- Quantize toggle, Scale Select, MTS-ESP toggle

#### Tape DSP Engine (7 parameters)
- Tape Mode, Saturation, Aging, Instability, Noise Enable, Noise Amount, Head Config

#### Modulation (Wow & Flutter) (6 parameters)
- Wow Rate/Depth, Flutter Rate/Depth, Wow/Flutter Waveform selectors

#### Head EQ System (4 parameters)
- Head Bump Freq/Gain, Rolloff Freq/Resonance

#### Gain Controls (2 parameters)
- Input Gain, Output Gain

### 📶 CV Control Implementation

All 21 CV inputs implemented with proper ±10V scaling:
- Input/Output Gain CVs
- Time L/R, Feedback L/R, Mix L/R CVs  
- Global Time/Feedback Mod CVs
- Pitch, Detune L/R, Detune Drift CVs
- Morph, Character CVs
- Wow/Flutter Rate/Depth CVs
- Saturation, Aging, Instability CVs
- Tape Noise, Noise Amount, Head Select CVs

### 🔄 Signal Flow Architecture

Complete signal processing chain:
```
Audio Input → Input Gain → Pitch Shifting → Delay Lines → Cross-Feedback → Tape Processing → Output Mix → Audio Output
```

- Independent stereo processing throughout entire chain
- Cross-feedback applied after delay but before tape processing  
- Pitch shifting with automatic gain compensation
- Tape effects as final processing stage before output mixing
- Real-time level metering with 5-segment displays

### 🎯 Quality & Performance

#### Stability Features
- Enhanced gain staging throughout signal path
- Improved soft clipping algorithms for musical limiting
- Advanced interpolation for fractional delay lines
- Optimized filter coefficient updates with stability checking
- Buffer overflow protection in all audio processing

#### Performance Optimizations
- Efficient lookup tables for real-time performance
- Optimized sample rate handling with proper coefficient updates
- Dynamic buffer management with safety bounds
- Lock-free CV processing in audio thread

### 📦 Distribution & Installation

#### Package Contents
- `plugin.dll` - Main plugin binary (2.6MB)
- `plugin.json` - Plugin metadata and configuration
- `res/CurveAndDrag.svg` - Professional 48HP panel layout (25KB)
- `LICENSE` - MIT license
- `README.md` - Complete documentation

#### Supported Platforms
- **Windows x64**: Fully tested and optimized
- **VCV Rack 2.0+**: Compatible with latest VCV Rack versions
- **SIMD Optimization**: Targeted instruction sets for broad hardware compatibility

### ⚠️ Known Limitations

#### Performance Considerations
- Large pitch shifts (>±1 octave) may introduce latency artifacts
- High feedback (>90%) requires careful gain staging
- CPU usage increases with tape mode and heavy modulation

#### Platform-Specific Notes
- MTS-ESP requires separate host plugin for external tuning
- Scala files limited to 12-tone octave patterns
- Pitch tracking optimized for tonal material

---

*CurveAndDrag v1.0 represents the complete initial professional release with all planned features implemented and tested.* 