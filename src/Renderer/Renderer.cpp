#include "Renderer.hpp"
#include "Core/Defines.hpp"

#include "Renderer/Backend/RenderPanic.hpp"

RendererStateInstance RendererState;

AR_SET_MODULE_NAME("Renderer")

void AssertRendererExists()
{
    if (!RendererState.Vulkan.Initialized) {
        Panic("RendererState not initialized!", 0);
    }
}
