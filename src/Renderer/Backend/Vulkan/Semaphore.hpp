#pragma once

#include "Renderer/Backend/RenderPanic.hpp"
#include "Device.hpp"
#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.h>

namespace vulkan {

class Semaphore
{
public:
    void Create(GPUDevice *device)
    {
        const VkSemaphoreCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .flags = 0,
            .pNext = nullptr,
        };

        this->mDevice = device;

        const VkResult status = vkCreateSemaphore(device->Device, &create_info, nullptr, &this->Semaphore);

        if (status != VK_SUCCESS) {
            Panic_("Semaphore", "Could not create semaphore", status);
        }
    }

    void Destroy()
    {
        vkDestroySemaphore(this->mDevice->Device, this->Semaphore, nullptr);
    }

public:
    VkSemaphore Semaphore;
private:
    GPUDevice *mDevice = nullptr;
};

};
