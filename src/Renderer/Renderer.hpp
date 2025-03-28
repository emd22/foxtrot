#pragma once

#include <Renderer/Backend/FxRenderBackendVulkan.hpp>
#include <Renderer/FxRenderBackend.hpp>

void SetRendererBackend(FxRenderBackend *backend);

void AssertRendererExists();

extern FxRenderBackend *Renderer;
extern FxRenderBackendVulkan *RendererVulkan;
