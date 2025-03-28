#pragma once

#include <Math/Vec2.hpp>

#include <Renderer/FxWindow.hpp>

// TODO: make base versions of these components
#include <Renderer/Backend/Vulkan/Pipeline.hpp>
#include <Renderer/Backend/Vulkan/Swapchain.hpp>
#include <Renderer/Backend/Vulkan/FrameData.hpp>

#include <ThirdParty/vk_mem_alloc.h>

#include <memory>

using namespace vulkan;

enum class FrameResult
{
    Success,
    GraphicsOutOfDate,
    RenderError,
};

class FxRenderBackend {
public:
    virtual void Init(Vec2i window_size) = 0;
    virtual void Destroy() = 0;

    virtual void SelectWindow(std::shared_ptr<FxWindow> window) = 0;

    virtual FrameResult BeginFrame(GraphicsPipeline &pipeline) = 0;
    virtual void FinishFrame(GraphicsPipeline &pipeline) = 0;

    virtual ~FxRenderBackend() = default;

public:
    bool Initialized = false;
};
