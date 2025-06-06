# Contributing to Curve&Drag

Thank you for your interest in contributing to Curve&Drag! This document provides guidelines and information for contributors.

## Code of Conduct

We are committed to providing a welcoming and inclusive environment for all contributors. Please be respectful and constructive in all interactions.

## Getting Started

### Development Environment Setup

1. **Install VCV Rack SDK**:
   ```bash
   # Download VCV Rack SDK from https://vcvrack.com/downloads/
   # Extract and set RACK_DIR environment variable
   export RACK_DIR=/path/to/Rack-SDK
   ```

2. **Clone the Repository**:
   ```bash
   git clone https://github.com/your-username/Curve-and-Drag.git
   cd Curve-and-Drag
   ```

3. **Build the Plugin**:
   ```bash
   make clean
   make
   ```

4. **Install for Testing**:
   ```bash
   make install
   ```

### Development Dependencies

- **C++ Compiler**: GCC 7+ or Clang 6+ with C++17 support
- **VCV Rack SDK**: Version 2.0 or later
- **Git**: For version control
- **Make**: For building

## Contribution Guidelines

### Types of Contributions

We welcome various types of contributions:

- **Bug Reports**: Help us identify and fix issues
- **Feature Requests**: Suggest new functionality
- **Code Contributions**: Bug fixes, new features, optimizations
- **Documentation**: Improve README, comments, or create tutorials
- **Testing**: Test on different platforms and configurations

### Before Contributing

1. **Check Existing Issues**: Look for existing bug reports or feature requests
2. **Discuss Major Changes**: Open an issue to discuss significant changes before implementing
3. **Follow Coding Standards**: Ensure your code follows our style guidelines

## Coding Standards

### C++ Style Guidelines

1. **Naming Conventions**:
   ```cpp
   // Classes: PascalCase
   class DelayLine {};
   
   // Functions and variables: camelCase
   void processAudio();
   float sampleRate;
   
   // Constants: UPPER_SNAKE_CASE
   static constexpr int MAX_DELAY_TIME = 2000;
   
   // Private members: camelCase with trailing underscore
   float delayTime_;
   ```

2. **Code Formatting**:
   - Use 4 spaces for indentation (no tabs)
   - Maximum line length: 100 characters
   - Place opening braces on the same line
   - Use consistent spacing around operators

3. **Documentation**:
   ```cpp
   /**
    * @brief Brief description of the function
    * @param paramName Description of parameter
    * @return Description of return value
    */
   float processDelay(float input, float delayTime);
   ```

### File Organization

- **Header Files**: Use `.hpp` extension
- **Source Files**: Use `.cpp` extension
- **Include Guards**: Use `#pragma once`
- **Includes**: Group system includes, then VCV Rack includes, then local includes

### Performance Guidelines

1. **Audio Thread Safety**:
   - Avoid memory allocation in `process()` methods
   - Use lock-free data structures when possible
   - Minimize expensive operations in audio processing

2. **Memory Management**:
   - Use RAII principles
   - Prefer smart pointers over raw pointers
   - Avoid memory leaks

3. **Optimization**:
   - Profile before optimizing
   - Use appropriate data types
   - Consider SIMD optimizations for DSP code

## Submitting Changes

### Pull Request Process

1. **Fork the Repository**: Create your own fork on GitHub

2. **Create a Feature Branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make Your Changes**:
   - Write clean, well-documented code
   - Add tests if applicable
   - Update documentation as needed

4. **Test Your Changes**:
   ```bash
   make clean
   make
   make install
   # Test in VCV Rack
   ```

5. **Commit Your Changes**:
   ```bash
   git add .
   git commit -m "feat: add new feature description"
   ```

6. **Push to Your Fork**:
   ```bash
   git push origin feature/your-feature-name
   ```

7. **Create Pull Request**: Open a PR on GitHub with a clear description

### Commit Message Format

Use conventional commit format:

```
type(scope): description

[optional body]

[optional footer]
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

Examples:
```
feat(delay): add cross-feedback functionality
fix(mts): resolve memory leak in MTS client
docs(readme): update installation instructions
```

### Pull Request Guidelines

- **Clear Description**: Explain what changes you made and why
- **Link Issues**: Reference any related issues
- **Test Results**: Describe testing performed
- **Breaking Changes**: Clearly mark any breaking changes
- **Screenshots**: Include screenshots for UI changes

## Testing

### Manual Testing

1. **Basic Functionality**:
   - Test all parameters and CV inputs
   - Verify audio processing quality
   - Check for audio dropouts or glitches

2. **Edge Cases**:
   - Test with extreme parameter values
   - Test with no input signal
   - Test with very loud input signals

3. **Platform Testing**:
   - Test on Windows, macOS, and Linux if possible
   - Test with different sample rates
   - Test with different buffer sizes

### Automated Testing

While we don't currently have a comprehensive test suite, we encourage:

- Unit tests for utility functions
- Integration tests for DSP algorithms
- Performance benchmarks for critical paths

## Documentation

### Code Documentation

- Use Doxygen-style comments for public APIs
- Document complex algorithms and their sources
- Include usage examples for non-obvious functions

### User Documentation

- Update README.md for new features
- Add parameter descriptions to the manual
- Create usage examples and tutorials

## Release Process

### Version Numbering

We use Semantic Versioning (SemVer):
- **Major**: Breaking changes
- **Minor**: New features (backward compatible)
- **Patch**: Bug fixes (backward compatible)

### Release Checklist

1. Update version in `plugin.json`
2. Update CHANGELOG.md
3. Test thoroughly on all platforms
4. Create release notes
5. Tag the release
6. Build and distribute binaries

## Getting Help

### Communication Channels

- **GitHub Issues**: For bug reports and feature requests
- **GitHub Discussions**: For general questions and ideas
- **VCV Rack Community Forum**: For broader community discussion

### Resources

- [VCV Rack Plugin Development Tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial)
- [VCV Rack API Documentation](https://vcvrack.com/docs/)
- [MTS-ESP Documentation](https://oddsound.com/mts-esp/)

## Recognition

Contributors will be recognized in:
- CHANGELOG.md for their contributions
- README.md acknowledgments section
- Git commit history

## License

By contributing to Curve&Drag, you agree that your contributions will be licensed under the GPL-3.0-or-later license.

---

Thank you for contributing to Curve&Drag! Your efforts help make this a better tool for the VCV Rack community. 