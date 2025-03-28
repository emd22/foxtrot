#include "Renderer.hpp"
#include "Core/Defines.hpp"

#include <Renderer/FxRenderBackend.hpp>
#include "Renderer/Backend/RenderPanic.hpp"

AR_SET_MODULE_NAME("Renderer")

FxRenderBackend *Renderer;
VkRenderBackend *RendererVulkan;

void SetRendererBackend(FxRenderBackend *backend)
{
    Renderer = backend;
    RendererVulkan = static_cast<VkRenderBackend *>(Renderer);
}

void AssertRendererExists()
{
    if (!Renderer->Initialized) {
        Panic("Renderer not initialized!", 0);
    }
}
