#pragma once

#include "lib/def.h"
#include "lib/file.h"
#include "lib/string.h"
#include <SDL3/SDL.h>

// Function to create and compile shader from file
SDL_GPUShader* load_shader(
    Allocator& allocator,
    SDL_GPUDevice* device,
    string shader_name,
    u32 num_samplers,
    u32 num_uniform_buffers,
    u32 num_storage_buffers,
    u32 num_storage_textures
) {
    SDL_GPUShaderStage stage;
    if (string_contains(shader_name, ".vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } else if (string_contains(shader_name, ".frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } else {
        SDL_Log("Unsupported shader file type: %s\n", shader_name);
        return nullptr;
    }

    SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    char extension[8] = "";
    char entrypoint[16] = "main";

    if ((backend_formats & SDL_GPU_SHADERFORMAT_SPIRV) != 0) {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        strncpy(extension, ".spv", sizeof(extension));
    } else if ((backend_formats & SDL_GPU_SHADERFORMAT_DXIL) != 0) {
        format = SDL_GPU_SHADERFORMAT_DXIL;
        strncpy(extension, ".dxil", sizeof(extension));
    } else if ((backend_formats & SDL_GPU_SHADERFORMAT_MSL) != 0) {
        format = SDL_GPU_SHADERFORMAT_MSL;
        strncpy(extension, ".msl", sizeof(extension));
        strncpy(entrypoint, "main0", sizeof(entrypoint));
    } else {
        SDL_Log("No supported shader formats available");
        return nullptr;
    }

    char shader_path[1024];
    i32 result =
        snprintf(shader_path, sizeof(shader_path), "assets/shaders/compiled/%s%s", shader_name, extension);

    if (result < 0 || result >= (i32)sizeof(shader_path)) {
        SDL_Log("Shader path too long or formatting error\n");
        return nullptr;
    }

    auto file = File::read_all(allocator, shader_path);
    if (!file.has_value()) {
        SDL_Log("Failed to read shader file: %s\n", shader_path);
        return nullptr;
    }
    defer { file->deinit(); };

    SDL_GPUShaderCreateInfo create_info = {
        .code_size = file->size,
        .code = (u8*)file->c_str(),
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = num_samplers,
        .num_storage_textures = num_storage_textures,
        .num_storage_buffers = num_storage_buffers,
        .num_uniform_buffers = num_uniform_buffers,
        .props = 0,
    };

    return SDL_CreateGPUShader(device, &create_info);
}