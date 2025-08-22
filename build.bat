@echo off
setlocal enabledelayedexpansion

:: Set project directories
set PROJECT_ROOT=%~dp0
set SRC_DIR=%PROJECT_ROOT%src
set BUILD_DIR=%PROJECT_ROOT%build
set ASSETS_DIR=%PROJECT_ROOT%assets
set SHADER_SOURCE_DIR=%ASSETS_DIR%\shaders\source
set SHADER_OUT_DIR=%ASSETS_DIR%\shaders\compiled

:: Set SDL3 paths
set SDL3_DIR=C:\SDKs\SDL3
set SDL3_BUILD_DIR=%SDL3_DIR%\build\Release
set SDL3_INCLUDE_DIR=%SDL3_DIR%\include
set SDL3_LIB_DIR=%SDL3_BUILD_DIR%

:: Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: Create shader output directory if it doesn't exist
if not exist "%SHADER_OUT_DIR%" mkdir "%SHADER_OUT_DIR%"

:: Check if shadercross is available
where shadercross >nul 2>&1
if %errorlevel% equ 0 (
    echo Compiling shaders...

    :: Check if shader source directory exists
     if exist "%SHADER_SOURCE_DIR%" (
        :: Process .vert.hlsl files
        for %%f in ("%SHADER_SOURCE_DIR%\*.vert.hlsl") do (
            set "filename=%%~nf"
            set "basename=!filename:~0,-5!"
            echo Compiling vertex shader: %%~nxf
            shadercross "%%f" -o "%SHADER_OUT_DIR%\!basename!.vert.spv"
            shadercross "%%f" -o "%SHADER_OUT_DIR%\!basename!.vert.msl"
            shadercross "%%f" -o "%SHADER_OUT_DIR%\!basename!.vert.dxil"
        )
        
        :: Process .frag.hlsl files
        for %%f in ("%SHADER_SOURCE_DIR%\*.frag.hlsl") do (
            set "filename=%%~nf"
            set "basename=!filename:~0,-5!"
            echo Compiling fragment shader: %%~nxf
            shadercross "%%f" -o "%SHADER_OUT_DIR%\!basename!.frag.spv"
            shadercross "%%f" -o "%SHADER_OUT_DIR%\!basename!.frag.msl"
            shadercross "%%f" -o "%SHADER_OUT_DIR%\!basename!.frag.dxil"
        )
        
        :: Process .comp.hlsl files
        for %%f in ("%SHADER_SOURCE_DIR%\*.comp.hlsl") do (
            set "filename=%%~nf"
            set "basename=!filename:~0,-5!"
            echo Compiling compute shader: %%~nxf
            shadercross "%%f" -o "%SHADER_OUT_DIR%\!basename!.comp.spv"
            shadercross "%%f" -o "%SHADER_OUT_DIR%\!basename!.comp.msl"
            shadercross "%%f" -o "%SHADER_OUT_DIR%\!basename!.comp.dxil"
        )
        
        echo Shader compilation completed.
    ) else (
        echo Shader source directory not found: %SHADER_SOURCE_DIR%
    )
) else (
    echo Shadercross not found - Skipping shader compilation.
)

:: Compile the application
echo Compiling application...

clang++ -std=c++23 ^
    -g ^
    -Wall ^
    -Wextra ^
    -Wpedantic ^
    -Wno-c99-extensions ^
    -Wno-missing-designated-field-initializers ^
    -I%SDL3_INCLUDE_DIR% ^
    -L%SDL3_LIB_DIR% ^
    -o %BUILD_DIR%\main.exe ^
    %SRC_DIR%\main.cpp ^
    -Wl,/SUBSYSTEM:WINDOWS ^
    -lSDL3

if errorlevel 1 (
    echo Compilation failed
    exit /b 1
)

echo Build completed successfully!
echo Executable: %BUILD_DIR%\main.exe