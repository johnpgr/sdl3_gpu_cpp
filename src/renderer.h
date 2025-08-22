#pragma once

#include "lib/allocator.h"
#include "lib/def.h"
#include "math.h"
#include "shader.h"
#include <SDL3/SDL.h>
#include <expected>

[[maybe_unused]]
constexpr SDL_FColor COLOR_WHITE = {1.0f, 1.0f, 1.0f, 1.0f};
[[maybe_unused]]
constexpr SDL_FColor COLOR_BLACK = {0.0f, 0.0f, 0.0f, 1.0f};
[[maybe_unused]]
constexpr SDL_FColor COLOR_RED = {1.0f, 0.0f, 0.0f, 1.0f};
[[maybe_unused]]
constexpr SDL_FColor COLOR_GREEN = {0.0f, 1.0f, 0.0f, 1.0f};
[[maybe_unused]]
constexpr SDL_FColor COLOR_BLUE = {0.0f, 0.0f, 1.0f, 1.0f};
[[maybe_unused]]
constexpr SDL_FColor COLOR_CYAN = {0.0f, 1.0f, 1.0f, 1.0f};
[[maybe_unused]]
constexpr SDL_FColor COLOR_YELLOW = {1.0f, 1.0f, 0.0f, 1.0f};
[[maybe_unused]]
constexpr SDL_FColor COLOR_PINK = {1.0f, 0.0f, 1.0f, 1.0f};

struct VertexData {
    Vec4 pos;
    Vec4 color;
};

struct TransformBuffer {
    Mat4x4 mvp_matrix;
};

enum RendererInitError {
    VERTEX_SHADER_LOAD_ERROR,
    FRAGMENT_SHADER_LOAD_ERROR,
    PIPELINE_CREATION_ERROR,
    VERTEX_BUFFER_CREATION_ERROR,
    TRANSFER_BUFFER_CREATION_ERROR
};

constexpr string to_string(RendererInitError error) {
    switch (error) {
        case VERTEX_SHADER_LOAD_ERROR:
            return "VERTEX_SHADER_LOAD_ERROR";
        case FRAGMENT_SHADER_LOAD_ERROR:
            return "FRAGMENT_SHADER_LOAD_ERROR";
        case PIPELINE_CREATION_ERROR:
            return "PIPELINE_CREATION_ERROR";
        case VERTEX_BUFFER_CREATION_ERROR:
            return "VERTEX_BUFFER_CREATION_ERROR";
        case TRANSFER_BUFFER_CREATION_ERROR:
            return "TRANSFER_BUFFER_CREATION_ERROR";
        default:
            return "UNKNOWN_ERROR";
    }
}

struct Renderer {
    Allocator& allocator;
    SDL_GPUDevice* device{};
    SDL_GPUGraphicsPipeline* pipeline_fill{};
    SDL_GPUGraphicsPipeline* pipeline_line{};
    SDL_GPUBuffer* vertex_buffer{};

    static std::expected<Renderer, RendererInitError>
    init(Allocator& allocator, SDL_GPUDevice* device, SDL_Window* window) {
        SDL_GPUShader* vertex_shader =
            load_shader(allocator, device, "raw-triangle.vert", 0, 1, 0, 0);
        SDL_GPUShader* fragment_shader =
            load_shader(allocator, device, "solid-color.frag", 0, 0, 0, 0);

        if (!vertex_shader || !fragment_shader) {
            SDL_Log("Failed to load shaders %s\n", SDL_GetError());
            return std::unexpected(VERTEX_SHADER_LOAD_ERROR);
        }

        defer {
            SDL_ReleaseGPUShader(device, vertex_shader);
            SDL_ReleaseGPUShader(device, fragment_shader);
        };

        SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {
            .vertex_shader = vertex_shader,
            .fragment_shader = fragment_shader,
            .vertex_input_state =
                (SDL_GPUVertexInputState){
                    .vertex_buffer_descriptions =
                        (SDL_GPUVertexBufferDescription[]){
                            {
                                .slot = 0,
                                .pitch = sizeof(VertexData),
                                .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
                            },
                        },
                    .num_vertex_buffers = 1,
                    .vertex_attributes =
                        (SDL_GPUVertexAttribute[]){
                            {
                                .location = 0,
                                .buffer_slot = 0,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                                .offset = 0,
                            },
                            {
                                .location = 1,
                                .buffer_slot = 0,
                                .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
                                .offset = sizeof(f32) * 4,
                            },
                        },
                    .num_vertex_attributes = 2,
                },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .target_info = {
                .color_target_descriptions =
                    (SDL_GPUColorTargetDescription[]){
                        {
                            .format = SDL_GetGPUSwapchainTextureFormat(device, window),
                        },
                    },
                .num_color_targets = 1,
            },
        };

        pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        SDL_GPUGraphicsPipeline* pipeline_fill =
            SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        if (!pipeline_fill) {
            SDL_Log("Failed to create graphics pipeline %s\n", SDL_GetError());
            return std::unexpected(PIPELINE_CREATION_ERROR);
        }

        pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
        SDL_GPUGraphicsPipeline* pipeline_line =
            SDL_CreateGPUGraphicsPipeline(device, &pipeline_info);
        if (!pipeline_line) {
            SDL_Log("Failed to create graphics pipeline %s\n", SDL_GetError());
            return std::unexpected(PIPELINE_CREATION_ERROR);
        }

        VertexData triangle_vertices[] = {
            {
                .pos = Vec4::init(-0.5f, -0.5f, 0.0f, 1.0f),
                .color = Vec4::init(1.0f, 0.0f, 0.0f, 1.0f),
            },
            {
                .pos = Vec4::init(0.5f, -0.5f, 0.0f, 1.0f),
                .color = Vec4::init(0.5f, 1.0f, 0.0f, 1.0f),
            },
            {
                .pos = Vec4::init(0.0f, 0.5f, 0.0f, 1.0f),
                .color = Vec4::init(0.0f, 0.0f, 1.0f, 1.0f),
            },
        };

        SDL_GPUBufferCreateInfo vertex_buffer_info = {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(triangle_vertices),
        };

        SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
        if (!vertex_buffer) {
            SDL_Log("Failed to create vertex buffer %s\n", SDL_GetError());
            return std::unexpected(VERTEX_BUFFER_CREATION_ERROR);
        }

        SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
            device,
            &(SDL_GPUTransferBufferCreateInfo){
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = sizeof(triangle_vertices),
            }
        );
        if (!transfer_buffer) {
            SDL_Log("Failed to create transfer buffer %s\n", SDL_GetError());
            return std::unexpected(TRANSFER_BUFFER_CREATION_ERROR);
        }
        defer { SDL_ReleaseGPUTransferBuffer(device, transfer_buffer); };

        void* transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
        memcpy(transfer_data, triangle_vertices, sizeof(triangle_vertices));
        SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

        SDL_GPUCommandBuffer* upload_cmdbuf = SDL_AcquireGPUCommandBuffer(device);
        SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_cmdbuf);
        SDL_UploadToGPUBuffer(
            copy_pass,
            &(SDL_GPUTransferBufferLocation){
                .transfer_buffer = transfer_buffer,
                .offset = 0,
            },
            &(SDL_GPUBufferRegion){
                .buffer = vertex_buffer,
                .offset = 0,
                .size = sizeof(triangle_vertices),
            },
            false
        );
        SDL_EndGPUCopyPass(copy_pass);
        SDL_SubmitGPUCommandBuffer(upload_cmdbuf);

        return Renderer{
            .allocator = allocator,
            .device = device,
            .pipeline_fill = pipeline_fill,
            .pipeline_line = pipeline_line,
            .vertex_buffer = vertex_buffer,
        };
    }

    void deinit() {
        SDL_ReleaseGPUBuffer(device, vertex_buffer);
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline_fill);
        SDL_ReleaseGPUGraphicsPipeline(device, pipeline_line);
    }

    bool render(SDL_Window* window, i32 window_width, i32 window_height) {
        f32 aspect_ratio = (f32)window_width / (f32)window_height;
        Mat4x4 projection =
            Mat4x4::orthographic(-aspect_ratio, aspect_ratio, -1.0f, 1.0f, -1.0f, 1.0f);

        f32 time = SDL_GetTicks() / 1000.0f;
        Mat4x4 rotation = Mat4x4::rotation_z(time);
        Mat4x4 scale = Mat4x4::scale(0.8f, 0.8f, 1.0f);
        Mat4x4 model = scale * rotation;

        Mat4x4 mvp = projection * model;
        TransformBuffer transform_buffer = {.mvp_matrix = mvp};

        SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(device);
        if (!cmdbuf) {
            SDL_Log("Failed to acquire command buffer %s\n", SDL_GetError());
            return false;
        }

        SDL_GPUTexture* swapchain_texture;
        if (!SDL_WaitAndAcquireGPUSwapchainTexture(
                cmdbuf,
                window,
                &swapchain_texture,
                nullptr,
                nullptr
            )) {
            SDL_Log("Failed to acquire swapchain texture %s\n", SDL_GetError());
            return false;
        }

        if (!swapchain_texture) {
            SDL_Log("Swapchain texture is null\n");
            SDL_SubmitGPUCommandBuffer(cmdbuf);
            return false;
        }

        SDL_PushGPUVertexUniformData(cmdbuf, 0, &transform_buffer, sizeof(TransformBuffer));

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(
            cmdbuf,
            &(SDL_GPUColorTargetInfo){
                .texture = swapchain_texture,
                .clear_color = COLOR_BLACK,
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE,
            },
            1,
            nullptr
        );
        SDL_BindGPUGraphicsPipeline(render_pass, pipeline_fill);

        SDL_BindGPUVertexBuffers(
            render_pass,
            0,
            (SDL_GPUBufferBinding[]){
                {
                    .buffer = vertex_buffer,
                    .offset = 0,
                },
            },
            1
        );

        SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
        SDL_EndGPURenderPass(render_pass);
        SDL_SubmitGPUCommandBuffer(cmdbuf);

        return true;
    }
};