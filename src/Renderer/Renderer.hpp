#pragma once

#include <Renderer/Backend/Vulkan.hpp>
#include <Renderer/FxRenderBackend.hpp>

void SetRendererBackend(FxRenderBackend *backend);

void AssertRendererExists();

extern FxRenderBackend *Renderer;
extern VkRenderBackend *RendererVulkan;
