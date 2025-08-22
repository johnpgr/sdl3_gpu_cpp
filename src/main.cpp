#include "lib/allocator.h"
#include "lib/def.h"
#include "lib/file.h"
#include "lib/string.h"
#include "shader.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

struct Game {
    Allocator& allocator;
    SDL_Window* window = nullptr;
    SDL_GPUDevice* device = nullptr;
    SDL_Texture* main_texture = nullptr;
    SDL_GPUGraphicsPipeline* pipeline_fill = nullptr;
    SDL_GPUGraphicsPipeline* pipeline_line = nullptr;
    bool running = true;
    i32 window_width = 800;
    i32 window_height = 600;
};

SDL_FColor COLOR_WHITE = (SDL_FColor){1.0f, 1.0f, 1.0f, 1.0f};
SDL_FColor COLOR_BLACK = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};
SDL_FColor COLOR_RED = (SDL_FColor){1.0f, 0.0f, 0.0f, 1.0f};
SDL_FColor COLOR_GREEN = (SDL_FColor){0.0f, 1.0f, 0.0f, 1.0f};
SDL_FColor COLOR_BLUE = (SDL_FColor){0.0f, 0.0f, 1.0f, 1.0f};
SDL_FColor COLOR_CYAN = (SDL_FColor){0.0f, 1.0f, 1.0f, 1.0f};
SDL_FColor COLOR_YELLOW = (SDL_FColor){1.0f, 1.0f, 0.0f, 1.0f};
SDL_FColor COLOR_PINK = (SDL_FColor){1.0f, 0.0f, 1.0f, 1.0f};

bool init(Game* game) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Failed to start SDL %s\n", SDL_GetError());
        return false;
    }

    game->window = SDL_CreateWindow("Unnamed game", game->window_width, game->window_height, 0);
    if (!game->window) {
        SDL_Log("Failed to create window %s\n", SDL_GetError());
        return false;
    }

    game->device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL,
        true,
        nullptr
    );

    if (!game->device) {
        SDL_Log("Failed to create GPU Device %s\n", SDL_GetError());
        return false;
    }

    string device_driver = SDL_GetGPUDeviceDriver(game->device);
    SDL_Log("Created GPU Device with driver %s\n", device_driver);

    if (!SDL_ClaimWindowForGPUDevice(game->device, game->window)) {
        SDL_Log("Failed to claim window for GPU Device %s\n", SDL_GetError());
        return false;
    }

    SDL_GPUShader* vertex_shader =
        load_shader(game->allocator, game->device, "raw-triangle.vert", 0, 0, 0, 0);
    SDL_GPUShader* fragment_shader =
        load_shader(game->allocator, game->device, "solid-color.frag", 0, 0, 0, 0);

    if (!vertex_shader || !fragment_shader) {
        SDL_Log("Failed to load shaders %s\n", SDL_GetError());
        return false;
    }
    defer {
        SDL_ReleaseGPUShader(game->device, vertex_shader);
        SDL_ReleaseGPUShader(game->device, fragment_shader);
    };

    SDL_GPUColorTargetDescription color_target_descriptions[] = {{
        .format = SDL_GetGPUSwapchainTextureFormat(game->device, game->window),
    }};

    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.vertex_shader = vertex_shader;
    pipeline_info.fragment_shader = fragment_shader;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.target_info = {
        .color_target_descriptions = color_target_descriptions,
        .num_color_targets = 1,
    };

    pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    game->pipeline_fill = SDL_CreateGPUGraphicsPipeline(game->device, &pipeline_info);
    if (!game->pipeline_fill) {
        SDL_Log("Failed to create fill pipeline %s\n", SDL_GetError());
        return false;
    }

    pipeline_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
    game->pipeline_line = SDL_CreateGPUGraphicsPipeline(game->device, &pipeline_info);
    if (!game->pipeline_line) {
        SDL_Log("Failed to create line pipeline %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void deinit(Game* game) {
    SDL_ReleaseGPUGraphicsPipeline(game->device, game->pipeline_fill);
    SDL_ReleaseGPUGraphicsPipeline(game->device, game->pipeline_line);
    SDL_ReleaseWindowFromGPUDevice(game->device, game->window);
    SDL_DestroyWindow(game->window);
    SDL_DestroyGPUDevice(game->device);
    SDL_Quit();
}

void handle_event(SDL_Event* event, Game* game) {
    switch (event->type) {
        case SDL_EVENT_QUIT:
            game->running = false;
            break;
        case SDL_EVENT_KEY_DOWN:
            switch (event->key.key) {
                case SDLK_ESCAPE:
                    game->running = false;
                    break;
            }
            break;
    }
}

bool render(Game* game) {
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(game->device);
    if (!cmdbuf) {
        SDL_Log("Failed to acquire command buffer %s\n", SDL_GetError());
        return false;
    }

    SDL_GPUTexture* swapchain_texture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            cmdbuf,
            game->window,
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

    SDL_GPUColorTargetInfo color_target_info = {
        .texture = swapchain_texture,
        .clear_color = COLOR_BLACK,
        .load_op = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_STORE,
    };

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmdbuf, &color_target_info, 1, nullptr);
    SDL_BindGPUGraphicsPipeline(render_pass, game->pipeline_fill);
    SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(render_pass);

    SDL_SubmitGPUCommandBuffer(cmdbuf);
    return true;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    ArenaAllocator arena = ArenaAllocator::init(PageAllocator::init(), 4096, GB(2));
    Allocator allocator = arena.allocator();

    Game game = Game{.allocator = allocator};

    defer {
        deinit(&game);
        arena.deinit();
    };

    if (!init(&game)) {
        return -1;
    }

    while (game.running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            handle_event(&event, &game);
        }
        if (!render(&game)) {
            break;
        }
    }

    return 0;
}
