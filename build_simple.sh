#!/bin/bash
# Minimal Windows build script for CurveAndDrag
# Uses only essential headers to avoid MinGW conflicts

set -e

echo "Building CurveAndDrag for Windows (simple approach)..."

# Clean previous builds
rm -f plugin.dll

# Extremely conservative build - exclude problematic MinGW headers
x86_64-w64-mingw32-g++ \
    -std=c++11 \
    -I./src \
    -I../Rack-WIN-SDK/include \
    -DARCH_WIN \
    -DWIN32 \
    -D_WIN32 \
    -DNOMINMAX \
    -O1 \
    -fno-strict-aliasing \
    -shared \
    -o plugin.dll \
    src/plugin.cpp \
    src/CurveAndDrag.cpp \
    -L../Rack-WIN-SDK \
    -lRack \
    -Wl,--enable-stdcall-fixup

if [ -f plugin.dll ]; then
    echo "✅ Simple Windows build successful: plugin.dll ($(du -h plugin.dll | cut -f1))"
    file plugin.dll
else
    echo "❌ Build failed"
    exit 1
fi 