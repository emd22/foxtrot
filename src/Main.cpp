

#include "Core/Defines.hpp"
#include "Renderer/Backend/Vulkan.hpp"
#include "Renderer/Backend/Vulkan/Shader.hpp"
#include "Renderer/Backend/Vulkan/Shader.hpp"
#include "Renderer/Backend/Vulkan/ShaderList.hpp"
#include "vulkan/vulkan_core.h"

#define SDL_DISABLE_OLD_NAMES
#include <SDL3/SDL.h>
#include <SDL3/SDL_revision.h>

#include <SDL3/SDL_main.h>

#include <Core/Types.hpp>

#include <Renderer/Renderer.hpp>

#include <Renderer/FxWindow.hpp>
#include <Renderer/FxMesh.hpp>

static bool Running = true;

AR_SET_MODULE_NAME("Main")

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

    auto window = FxWindow::New("Foxtrot Engine", 1024, 720);

    vulkan::VkRenderBackend renderer_state;
    SetRendererBackend(&renderer_state);

    Renderer->SelectWindow(window);
    Renderer->Init(Vec2i(1024, 720));

    vulkan::GraphicsPipeline pipeline;

    {
        ShaderList shader_list;

        vulkan::Shader vertex_shader("../shaders/main.vert.spv", ShaderType::Vertex);
        vulkan::Shader fragment_shader("../shaders/main.frag.spv", ShaderType::Fragment);

        shader_list.Vertex = vertex_shader.ShaderModule;
        shader_list.Fragment = fragment_shader.ShaderModule;

        pipeline.Create(shader_list);

        RendererVulkan->Swapchain.CreateSwapchainFramebuffers(&pipeline);
    }

    StaticArray<Vertex> test_mesh_verts = {
        Vertex{ .Position = { -1,  1, 0 } },
        Vertex{ .Position = {  1,  1, 0 } },
        Vertex{ .Position = {  0,  -1, 0 } },
    };

    FxMesh test_mesh(test_mesh_verts);

    while (Running) {
        ProcessEvents();

        if (Renderer->BeginFrame(pipeline) != FrameResult::Success) {
            continue;
        }

        test_mesh.Render();

        Renderer->FinishFrame(pipeline);
    }

    return 0;
}
