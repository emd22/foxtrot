#include "Renderer.hpp"
#include "Core/Defines.hpp"

#include "Renderer/Backend/RenderPanic.hpp"

AR_SET_MODULE_NAME("Renderer")

RendererStateInstance *RendererState;

void SetRendererState(RendererStateInstance *instance)
{
    RendererState = instance;
}

void AssertRendererExists()
{
    if (!RendererState->Vulkan.Initialized) {
        Panic("RendererState not initialized!", 0);
    }
}
