#pragma once

#include <vulkan/vulkan.h>

namespace vulkan {

class Swapchain;
class GPUDevice;
class CommandBuffer;

class RenderPass
{
public:
    void Create(GPUDevice &device, Swapchain &swapchain);

    void Begin();
    void End();

    void Destroy(GPUDevice &device);

public:
    VkRenderPass RenderPass = nullptr;
    CommandBuffer *CommandBuffer = nullptr;
};

}; // namespace vulkan
