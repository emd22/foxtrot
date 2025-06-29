#pragma once

#include <vulkan/vulkan.h>

#include <Core/FxSizedArray.hpp>

class RvkSwapchain;
class RvkGpuDevice;
class RvkCommandBuffer;

class RvkRenderPass
{
public:
    void Create(RvkGpuDevice &device, RvkSwapchain &swapchain);

    void Begin();
    void End();

    void Destroy(RvkGpuDevice &device);

public:
    // FxSizedArray<VkRenderingInfo> RenderInfos;
    // FxSizedArray<VkRenderingAttachmentInfo> AttachmentInfos;

    // VkRenderPass RenderPass = nullptr;
    RvkCommandBuffer *CommandBuffer = nullptr;
};
