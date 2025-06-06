# Changelog

All notable changes to CurveAndDrag will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2024-12-03

### 🎉 Added

#### Advanced Tape Emulation System
- **Neural-inspired time-varying biquad filters** with coefficient drift simulation
- **Multi-resolution modulation system** with 6 independent oscillator layers:
  - Primary/secondary wow (0.1-3 Hz) with golden ratio complexity
  - Ultra-slow drift oscillator for long-term variation
  - Fast/medium/chaos flutter (3-20 Hz) with instability control
- **Dynamic vintage saturation** with lookup table hysteresis curves
- **True stereo processing** with L/R decorrelation and crosstalk simulation
- **Advanced noise shaping** (pink, brown, hum, crackle) with aging correlation
- **Fractional delay modulation** with cubic interpolation for smooth pitch warbling
- **Tape aging simulation** with frequency-dependent degradation
- **Bias modulation** and DC offset drift for authentic tape behavior

#### Professional Pitch Shifting
- **Four vintage pitch shift modes**:
  - BBD (Bucket Brigade Device) - analog delay line simulation  
  - H910 (Harmonizer) - classic Eventide algorithm
  - Varispeed - tape speed modulation effect
  - Hybrid - combination of multiple techniques
- **Per-channel detune control** (±50 cents) for stereo chorus effects
- **Morphing system** between raw and quantized pitch with smooth transitions
- **Enhanced pitch range** (±200 cents) for musical pitch shifting

#### Comprehensive Microtonal Support
- **MTS-ESP integration** with real-time tuning table updates
- **Scala file support** with .scl format parser and error handling
- **Built-in temperaments**: 12-TET, Just Intonation, Pythagorean, 24-TET, 31-TET, Bohlen-Pierce
- **Priority tuning system**: MTS-ESP → Scala → Built-in scales
- **Automatic quantization** with CV-controlled morphing

#### Advanced CV Control System
- **Comprehensive CV inputs** for all major parameters
- **Proper CV scaling** (±10V bipolar, 0-10V unipolar)
- **Real-time modulation** of delay times, feedback, pitch, and tape parameters
- **New tape aging CV inputs**: TAPE_AGE_CV and TAPE_INSTABILITY_CV
- **Enhanced parameter clamping** with musical ranges

#### User Interface Enhancements
- **Professional LED level meters** (5-segment per channel)
- **Status lights** for all major functions (tempo sync, tape mode, MTS connection)
- **Activity indicators** for wow/flutter modulation
- **Parameter display updates** showing current subdivision names and values
- **Visual feedback** for all switch states and connections

### 🔄 Changed

#### Core Architecture Overhaul
- **Completely rewritten TapeDelayProcessor** with neural-inspired algorithms
- **Simplified signal flow** for improved stability and performance
- **Enhanced buffer management** with dynamic sizing and safety bounds
- **Improved sample rate handling** with proper coefficient updates
- **Optimized lookup tables** for real-time performance (2048 entries)

#### Parameter Ranges and Scaling
- **Pitch range reduced** from ±1200 to ±200 cents for more musical control
- **Detune range standardized** to ±50 cents per channel
- **Feedback safety limit** reduced to 95% to prevent oscillation
- **Enhanced CV input scaling** with proper VCV Rack voltage standards
- **Improved parameter quantization** for discrete controls (subdivisions, vintage modes)

#### Audio Processing Improvements
- **Enhanced gain staging** throughout the signal path
- **Improved soft clipping** algorithms for musical limiting
- **Better cross-feedback implementation** with stability controls
- **Advanced interpolation** for fractional delay lines
- **Optimized filter coefficient updates** with stability checking

### 🐛 Fixed

#### Critical Stability Issues
- **Fixed tape mode control logic** - tape processing now properly toggles on/off
- **Resolved always-on tape bug** - delay now works correctly when tape mode is disabled
- **Eliminated audio spikes** from runaway saturation feedback loops
- **Fixed buffer overflow issues** in fractional delay processing
- **Corrected coefficient drift instability** causing filter explosions

#### Audio Quality Fixes
- **Enhanced saturation audibility** - now produces clear, musical compression at all levels
- **Fixed choppy wow/flutter** - replaced stepping artifacts with smooth warbling
- **Stabilized aging parameters** - tape age/instability no longer break signal
- **Improved fractional delay** - eliminated aliasing and discontinuities
- **Fixed cross-feedback oscillation** potential with simplified routing

#### Parameter and CV Issues  
- **Corrected CV input scaling** for proper ±10V range handling
- **Fixed parameter display updates** for tempo sync and subdivision modes
- **Resolved MTS-ESP connection detection** with proper client registration
- **Fixed Scala file loading** with comprehensive error handling
- **Corrected pitch quantization** priority system (MTS-ESP → Scala → 12-TET)

### 🗑️ Removed

#### Deprecated Features
- **Complex feedback tape processing** - removed for stability (may return in future)
- **Extreme coefficient drift ranges** - limited for musical results
- **Unsafe parameter ranges** - clamped to prevent system instability
- **Legacy analog coloration** when tape mode is enabled (redundant with tape processor)
- **Unused filter types** and experimental modulation modes

#### Development Artifacts
- **Debug output and test code** cleaned up for release
- **Experimental pitch algorithms** not ready for production
- **Redundant parameter controls** that conflicted with main functionality

### ⚠️ Known Issues

#### Current Limitations
- **Cross-feedback simplified** - complex stereo feedback routing removed for stability
- **High tape age values** (>80%) may produce extreme frequency filtering
- **CPU usage increases** significantly with tape mode and heavy modulation
- **Potential feedback oscillation** at very high feedback levels (>90%)
- **Limited real-time safety** - some parameter changes may cause brief audio glitches

#### Platform-Specific Issues  
- **Windows**: No known issues
- **macOS**: May require security approval for MTS-ESP integration
- **Linux**: Some distributions may need additional audio permissions

#### Development Areas
- **Coefficient drift algorithm** could be more sophisticated
- **Stereo correlation** in tape noise could be enhanced
- **Pitch shifting quality** could benefit from higher-order interpolation
- **Memory usage** optimization for longer delay times
- **GUI responsiveness** with heavy CV modulation

### 🔮 Future Roadmap

#### Planned Enhancements
- **Enhanced cross-feedback** with frequency-dependent routing
- **Additional vintage tape models** (different tape formulations)
- **Expanded pitch shifting algorithms** (granular, spectral)
- **Advanced modulation matrix** for parameter relationships
- **Preset system** for quick recall of complex settings

#### Under Consideration
- **Multi-tap delay modes** with independent processing per tap
- **Convolution-based tape modeling** for ultimate realism
- **Advanced flutter algorithms** based on real tape transport analysis
- **Integration with other microtuning standards** beyond MTS-ESP
- **Real-time spectrum analysis** and adaptive processing

---

## [1.x.x] - Legacy Versions

### [1.0.0] - Initial Release
- Basic stereo delay functionality
- Simple pitch shifting
- Rudimentary tape effects
- Limited CV control
- 12-TET tuning only

*Note: Version 1.x codebase was completely rewritten for 2.0.0 due to architectural limitations.*

---

## Development Notes

### Version 2.0.0 Development Process
- **8 major development phases** with iterative improvements
- **Extensive testing** with professional musicians and audio engineers  
- **Performance optimization** using lookup tables and efficient algorithms
- **Stability focus** after reports of audio spikes and parameter instability
- **Cross-platform testing** on Windows, macOS, and Linux

### Technical Debt Addressed
- **Memory management** improved with RAII and smart pointers where applicable
- **Buffer overflow protection** added throughout audio processing chain
- **Exception handling** enhanced for file operations and MTS-ESP integration
- **Code organization** improved with clear separation of concerns
- **Documentation** expanded with comprehensive inline comments

### Regression Testing
- **Automated testing** for core delay functionality
- **Manual testing** of all tape parameters and edge cases
- **Performance benchmarking** across different sample rates and buffer sizes
- **Compatibility testing** with various MTS-ESP software and Scala files
- **Stress testing** with extreme parameter values and rapid changes 