

#include "Core/Defines.hpp"
#include "Renderer/Backend/Vulkan/Shader.hpp"
#include "Renderer/Backend/Vulkan/Shader.hpp"
#include "Renderer/Backend/Vulkan/ShaderList.hpp"

#define SDL_DISABLE_OLD_NAMES
#include <SDL3/SDL.h>
#include <SDL3/SDL_revision.h>

#include <SDL3/SDL_main.h>

#include <Core/Types.hpp>

#include <Renderer/Renderer.hpp>

static bool Running = true;

AR_SET_MODULE_NAME("Main")

class Window {
public:
    Window(const std::string &title, int32 width, int32 height) {
        const uint64 window_flags = SDL_WINDOW_VULKAN;

        mWindow = SDL_CreateWindow(title.c_str(), width, height, window_flags);

        if (mWindow == nullptr) {
            Panic("Could not create SDL window (SDL err: %s)", SDL_GetError());
        }
    }

    SDL_Window *GetWindow() { return this->mWindow; }

    ~Window() {
        SDL_DestroyWindow(mWindow);
    }

private:
    SDL_Window *mWindow;
};


inline void ProcessEvents() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                Running = false;
                break;
            default:;
        }
    }
}

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        Panic("Could not initialize SDL! (SDL err: %s)\n", SDL_GetError());
    }

    Window window("Test window", 1024, 720);

    RendererState.Vulkan.SelectWindow(window.GetWindow());
    RendererState.Vulkan.Init(Vec2i(1024, 720));

    vulkan::GraphicsPipeline pipeline;


    {
        ShaderList shader_list;

        vulkan::Shader vertex_shader("../shaders/main.vert.spv", ShaderType::Vertex);
        vulkan::Shader fragment_shader("../shaders/main.frag.spv", ShaderType::Fragment);

        shader_list.Vertex = vertex_shader.ShaderModule;
        shader_list.Fragment = fragment_shader.ShaderModule;

        pipeline.Create(shader_list);

        RendererState.Vulkan.Swapchain.CreateSwapchainFramebuffers(&pipeline);
    }


    while (Running) {
        ProcessEvents();
    }

    RendererState.Destroy();

    return 0;
}
