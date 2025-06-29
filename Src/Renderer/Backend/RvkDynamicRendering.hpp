#pragma once

#include <vulkan/vulkan.h>

#include <Core/FxSizedArray.hpp>

class RvkSwapchain;
class RvkGpuDevice;
class RvkCommandBuffer;

class RvkDynamicRendering
{
public:
    void Begin();
    void End();

public:
    RvkCommandBuffer *CommandBuffer = nullptr;
};
