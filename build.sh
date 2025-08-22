#!/bin/bash

# Set project directories
CC="clang++"
CFLAGS="-std=c++23 \
    -g \
    -Wall \
    -Wextra \
    -Wpedantic \
    -Wno-c99-extensions \
    -Wno-missing-designated-field-initializers \
    -Wno-reorder-init-list"

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$PROJECT_ROOT/src"
BUILD_DIR="$PROJECT_ROOT/build"
ASSETS_DIR="$PROJECT_ROOT/assets"
SHADER_SOURCE_DIR="$ASSETS_DIR/shaders/source"
SHADER_OUT_DIR="$ASSETS_DIR/shaders/compiled"

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"

# Create shader output directory if it doesn't exist
mkdir -p "$SHADER_OUT_DIR"

# Check if shadercross is available
if command -v shadercross &> /dev/null; then
    echo "Compiling shaders..."

    # Check if shader source directory exists
    if [ -d "$SHADER_SOURCE_DIR" ]; then
        # Process .vert.hlsl files
        for file in "$SHADER_SOURCE_DIR"/*.vert.hlsl; do
            if [ -f "$file" ]; then
                filename=$(basename "$file")
                basename="${filename%.vert.hlsl}"
                echo "Compiling vertex shader: $filename"
                shadercross "$file" -o "$SHADER_OUT_DIR/$basename.vert.spv"
                shadercross "$file" -o "$SHADER_OUT_DIR/$basename.vert.msl"
                shadercross "$file" -o "$SHADER_OUT_DIR/$basename.vert.dxil"
            fi
        done
        
        # Process .frag.hlsl files
        for file in "$SHADER_SOURCE_DIR"/*.frag.hlsl; do
            if [ -f "$file" ]; then
                filename=$(basename "$file")
                basename="${filename%.frag.hlsl}"
                echo "Compiling fragment shader: $filename"
                shadercross "$file" -o "$SHADER_OUT_DIR/$basename.frag.spv"
                shadercross "$file" -o "$SHADER_OUT_DIR/$basename.frag.msl"
                shadercross "$file" -o "$SHADER_OUT_DIR/$basename.frag.dxil"
            fi
        done
        
        # Process .comp.hlsl files
        for file in "$SHADER_SOURCE_DIR"/*.comp.hlsl; do
            if [ -f "$file" ]; then
                filename=$(basename "$file")
                basename="${filename%.comp.hlsl}"
                echo "Compiling compute shader: $filename"
                shadercross "$file" -o "$SHADER_OUT_DIR/$basename.comp.spv"
                shadercross "$file" -o "$SHADER_OUT_DIR/$basename.comp.msl"
                shadercross "$file" -o "$SHADER_OUT_DIR/$basename.comp.dxil"
            fi
        done
        
        echo "Shader compilation completed."
    else
        echo "Shader source directory not found: $SHADER_SOURCE_DIR"
    fi
else
    echo "Shadercross not found - Skipping shader compilation."
fi

# Check if pkg-config is available and can find SDL3
if command -v pkg-config &> /dev/null; then
    if pkg-config --exists sdl3; then
        SDL3_CFLAGS=$(pkg-config --cflags sdl3)
        SDL3_LIBS=$(pkg-config --libs sdl3)
    else
        SDL3_CFLAGS="-I/usr/local/include"
        SDL3_LIBS="-L/usr/local/lib -lSDL3"
    fi
else
    SDL3_CFLAGS="-I/usr/local/include"
    SDL3_LIBS="-L/usr/local/lib -lSDL3"
fi

# Compile the application
echo "Compiling application..."

$CC $CFLAGS \
    $SDL3_LIBS \
    $SDL3_CFLAGS \
    -o "$BUILD_DIR/main" \
    "$SRC_DIR/main.cpp"

if [ $? -ne 0 ]; then
    echo "Compilation failed"
    exit 1
fi

echo "Build completed successfully!"
echo "Executable: $BUILD_DIR/main"