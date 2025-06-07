#!/bin/bash
# ===== TARGETED WINDOWS BUILD FOR CURVEANDDRAG v1.0 =====
# Disables only the broken advanced SIMD instruction sets, keeps basic SIMD working

set -e

echo "🔧 Building CurveAndDrag v1.0 for Windows (Production Release)..."

# Clean previous builds
rm -f plugin.dll plugin.so *.vcvplugin

# ===== TARGETED FIX: Disable only broken instruction sets =====
# These specific instruction sets have broken vector types in MinGW
DISABLE_BROKEN_SIMD=(
    "-mno-avx512f"         # Disable AVX-512 Foundation
    "-mno-avx512cd"        # Disable AVX-512 Conflict Detection
    "-mno-avx512er"        # Disable AVX-512 Exponential and Reciprocal
    "-mno-avx512pf"        # Disable AVX-512 Prefetch
    "-mno-avx512dq"        # Disable AVX-512 Doubleword and Quadword
    "-mno-avx512bw"        # Disable AVX-512 Byte and Word
    "-mno-avx512vl"        # Disable AVX-512 Vector Length Extensions
    "-mno-avx512vbmi"      # Disable AVX-512 Vector Bit Manipulation
    "-mno-avx512ifma"      # Disable AVX-512 Integer Fused Multiply-Add
    "-mno-avx512vpopcntdq" # Disable AVX-512 Vector Population Count
    "-mno-fma4"            # Disable FMA4 (AMD-specific)
    "-mno-xop"             # Disable XOP (AMD-specific)
    "-mno-fma"             # Disable FMA3 (can cause issues)
    "-mno-f16c"            # Disable F16C (half-precision)
)

# Math constants (always required for Windows)
MATH_DEFINES="-DM_PI=3.14159265358979323846 -DM_SQRT2=1.41421356237309504880 -D_USE_MATH_DEFINES"

# Safe instruction sets that work with MinGW
SAFE_SIMD="-msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2"

# Architecture flags
ARCH_FLAGS="-march=x86-64 -mtune=generic"

# Combine flags
SIMD_FLAGS="${DISABLE_BROKEN_SIMD[*]} $SAFE_SIMD"
ALL_FLAGS="$MATH_DEFINES $SIMD_FLAGS $ARCH_FLAGS"

echo "Using targeted SIMD flags: $SIMD_FLAGS"

# ===== COMPILATION WITH TARGETED SIMD CONTROL =====
x86_64-w64-mingw32-g++ \
    -std=c++11 \
    -I./src \
    -I../Rack-WIN-SDK/include \
    -I../Rack-WIN-SDK/dep/include \
    $ALL_FLAGS \
    -O2 \
    -DARCH_WIN \
    -DWIN32 \
    -shared \
    -o plugin.dll \
    src/plugin.cpp \
    src/CurveAndDrag.cpp \
    -L../Rack-WIN-SDK \
    -lRack \
    -static-libgcc \
    -static-libstdc++ \
    2>&1

# Check build result
if [ -f plugin.dll ]; then
    echo "✅ CurveAndDrag v1.0 Windows build successful: plugin.dll ($(du -h plugin.dll | cut -f1))"
    
    # Create distribution package
    mkdir -p dist-windows
    cp plugin.dll dist-windows/
    cp plugin.json dist-windows/
    [ -d res ] && cp -r res dist-windows/
    [ -f LICENSE ] && cp LICENSE dist-windows/
    [ -f README.md ] && cp README.md dist-windows/
    
    # Create vcvplugin package for v1.0 release
    cd dist-windows
    zip -r "../CurveAndDrag-1.0-WINDOWS-x64.vcvplugin" *
    cd ..
    
    echo "📦 CurveAndDrag v1.0 Release Package Created: CurveAndDrag-1.0-WINDOWS-x64.vcvplugin"
    ls -la CurveAndDrag-1.0-WINDOWS-x64.vcvplugin
    echo ""
    echo "🚀 Ready for GitHub Release!"
    echo "   File: CurveAndDrag-1.0-WINDOWS-x64.vcvplugin"
    echo "   Size: $(du -h CurveAndDrag-1.0-WINDOWS-x64.vcvplugin | cut -f1)"
else
    echo "❌ CurveAndDrag v1.0 Windows build failed"
    exit 1
fi 