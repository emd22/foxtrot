#pragma once

#include <vulkan/vulkan.h>

#include <Core/FxSizedArray.hpp>

class RvkSwapchain;
class RvkGpuDevice;
class RvkCommandBuffer;

class RvkRenderPass
{
public:
    void Create(
        const RvkGpuDevice& device,
        const RvkSwapchain& swapchain,
        FxSizedArray<VkAttachmentDescription>& color_attachments,
        VkAttachmentDescription depth_attachment
    );

    void Begin();
    void End();

    void Destroy(RvkGpuDevice &device);

public:
    VkRenderPass RenderPass = nullptr;
    RvkCommandBuffer *CommandBuffer = nullptr;
};
