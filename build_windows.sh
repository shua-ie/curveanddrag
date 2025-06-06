#!/bin/bash
# Simple Windows build script for CurveAndDrag
# Avoids problematic AVX headers by using conservative flags

set -e

echo "Building CurveAndDrag for Windows..."

# Clean previous builds
rm -f plugin.dll

# Use conservative MinGW flags without AVX/SSE
x86_64-w64-mingw32-g++ \
    -std=c++17 \
    -I./src \
    -I../Rack-WIN-SDK/include \
    -I../Rack-WIN-SDK/dep/include \
    -O2 \
    -DARCH_WIN \
    -DWINVER=0x0601 \
    -D_WIN32_WINNT=0x0601 \
    -mno-sse \
    -mno-sse2 \
    -mno-sse3 \
    -mno-avx \
    -mno-avx2 \
    -shared \
    -o plugin.dll \
    src/CurveAndDrag.cpp \
    src/plugin.cpp \
    -L../Rack-WIN-SDK \
    -lRack \
    -static-libgcc \
    -static-libstdc++

if [ -f plugin.dll ]; then
    echo "✅ Windows build successful: plugin.dll ($(du -h plugin.dll | cut -f1))"
    file plugin.dll
else
    echo "❌ Build failed"
    exit 1
fi 