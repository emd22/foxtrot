#pragma once

#include "Renderer/Backend/RenderPanic.hpp"
#include "Device.hpp"

#include <vulkan/vulkan.h>
#include <cstdint>

namespace vulkan {

class Fence
{
public:
    void Create(GPUDevice *device)
    {
        const VkFenceCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
            .pNext = nullptr
        };

        this->mDevice = device;

        const VkResult status = vkCreateFence(this->mDevice->Device, &create_info, nullptr, &this->Fence);
        if (status != VK_SUCCESS) {
            Panic_("Fence", "Could not create fence", status);
        }
    }

    struct WaitOptions
    {
        uint64 Timeout = UINT64_MAX;
    };

    void WaitFor(WaitOptions wait_options = {.Timeout = UINT64_MAX}) const
    {
        const VkResult status = vkWaitForFences(this->mDevice->Device, 1, &this->Fence, true, wait_options.Timeout);

        if (status != VK_SUCCESS) {
            Panic_("Fence", "Could not create fence", status);
        }
    }

    void Destroy()
    {
        vkDestroyFence(this->mDevice->Device, this->Fence, nullptr);
    }

public:
    VkFence Fence;
private:
    GPUDevice *mDevice = nullptr;
};


}; // namespace vulkan
