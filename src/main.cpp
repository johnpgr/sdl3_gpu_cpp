#include "lib/allocator.h"
#include "lib/def.h"
#include "math.h"
#include "renderer.h"
#include "shader.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

enum GameInitError {
    SDL_INIT_FAILED,
    WINDOW_CREATION_FAILED,
    GPU_DEVICE_CREATION_FAILED,
    RENDERER_INIT_FAILED,
};

constexpr string to_string(GameInitError error) {
    switch (error) {
        case SDL_INIT_FAILED:
            return "SDL_INIT_FAILED";
        case WINDOW_CREATION_FAILED:
            return "WINDOW_CREATION_FAILED";
        case GPU_DEVICE_CREATION_FAILED:
            return "GPU_DEVICE_CREATION_FAILED";
        case RENDERER_INIT_FAILED:
            return "RENDERER_INIT_FAILED";
        default:
            return "UNKNOWN_ERROR";
    }
}

struct Game {
    Allocator& allocator;

    SDL_Window* window;
    SDL_GPUDevice* device;
    Renderer renderer;

    bool running;
    i32 window_width;
    i32 window_height;

    // FPS track
    u64 frame_count;
    u64 last_time;
    u64 fps_update_time;
    i32 current_fps;

    static std::expected<Game, GameInitError>
    init(Allocator& allocator, i32 window_width, i32 window_height) {
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            SDL_Log("Failed to start SDL %s\n", SDL_GetError());
            return std::unexpected(SDL_INIT_FAILED);
        }

        SDL_Window* window = SDL_CreateWindow(
            "FPS: 0",
            window_width,
            window_height,
            SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE
        );
        if (!window) {
            SDL_Log("Failed to create window %s\n", SDL_GetError());
            return std::unexpected(WINDOW_CREATION_FAILED);
        }

        SDL_GPUDevice* device = SDL_CreateGPUDevice(
            SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL,
            true,
            nullptr
        );
        if (!device) {
            SDL_Log("Failed to create GPU Device %s\n", SDL_GetError());
            return std::unexpected(GPU_DEVICE_CREATION_FAILED);
        }

        string device_driver = SDL_GetGPUDeviceDriver(device);
        SDL_Log("Created GPU Device with driver %s\n", device_driver);

        if (!SDL_ClaimWindowForGPUDevice(device, window)) {
            SDL_Log("Failed to claim window for GPU Device %s\n", SDL_GetError());
            return std::unexpected(WINDOW_CREATION_FAILED);
        }

        auto renderer = Renderer::init(allocator, device, window);
        if (!renderer.has_value()) {
            SDL_Log("Failed to initialize renderer: %s\n", to_string(renderer.error()));
            return std::unexpected(RENDERER_INIT_FAILED);
        }

        SDL_ShowWindow(window);

        u64 current_time = SDL_GetPerformanceCounter();

        return Game{
            .allocator = allocator,
            .window = window,
            .device = device,
            .renderer = renderer.value(),
            .running = true,
            .window_width = window_width,
            .window_height = window_height,
            .frame_count = 0,
            .last_time = current_time,
            .fps_update_time = current_time,
            .current_fps = 0
        };
    }

    void deinit() {
        renderer.deinit();
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyWindow(window);
        SDL_DestroyGPUDevice(device);
        SDL_Quit();
    }

    void update_fps() {
        frame_count++;
        u64 current_time = SDL_GetPerformanceCounter();
        u64 frequency = SDL_GetPerformanceFrequency();

        // Update FPS every second
        if (current_time - fps_update_time >= frequency) {
            f64 elapsed = (f64)(current_time - fps_update_time) / frequency;
            current_fps = (i32)(frame_count / elapsed);

            // Update window title
            char title[64];
            SDL_snprintf(title, sizeof(title), "FPS: %d", current_fps);
            SDL_SetWindowTitle(window, title);

            frame_count = 0;
            fps_update_time = current_time;
        }

        last_time = current_time;
    }

    void handle_event(SDL_Event* event) {
        switch (event->type) {
            case SDL_EVENT_QUIT:
                running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                switch (event->key.key) {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                }
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                // Update the stored window dimensions
                window_width = event->window.data1;
                window_height = event->window.data2;
                SDL_Log("Window resized to %dx%d\n", window_width, window_height);
                break;
        }
    }

    bool render() {
        return renderer.render(window, window_width, window_height);
    }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[]) {
    ArenaAllocator arena = ArenaAllocator::init(PageAllocator::init(), 4096, GB(2));
    Allocator allocator = arena.allocator();

    auto game = Game::init(allocator, 1280, 720);
    if (!game.has_value()) {
        SDL_Log("Failed to initialize game: %s\n", to_string(game.error()));
        return -1;
    }

    defer {
        game->deinit();
        arena.deinit();
    };

    while (game->running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            game->handle_event(&event);
        }
        if (!game->render()) {
            SDL_ShowSimpleMessageBox(
                0,
                "Render Error",
                "Failed to render frame. Check the console for details.",
                game->window
            );
        }
        game->update_fps();
    }

    return 0;
}