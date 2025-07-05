#pragma once

#include <vulkan/vulkan.h>

#include "RvkDevice.hpp"
#include "Fwd/Fwd_GetDevice.hpp"

#include <cassert>

class RvkSampler
{
public:
    void Create()
    {
        mDevice = Fx_Fwd_GetGpuDevice();
        assert(mDevice != nullptr);

        VkSamplerCreateInfo sampler_info{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
            .magFilter = VK_FILTER_LINEAR,
            .minFilter = VK_FILTER_LINEAR,

            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,

            .anisotropyEnable = VK_FALSE,
            // .maxAnisotropy = 0,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,

            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,

            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
            .mipLodBias = 0.0f,
            .minLod = 0.0f,
            .maxLod = 0.0f,
        };

        VkResult result = vkCreateSampler(mDevice->Device, &sampler_info, nullptr, &Sampler);

        if (result != VK_SUCCESS) {
            Log::Error("Error creating texture sampler!", 0);

            Sampler = nullptr;
        }
    }

    operator VkSampler() const
    {
        return Sampler;
    }


    void Destroy()
    {
        if (Sampler == nullptr) {
            return;
        }

        vkDestroySampler(mDevice->Device, Sampler, nullptr);

        Sampler = nullptr;
    }

    ~RvkSampler()
    {
        Destroy();
    }


public:
    VkSampler Sampler;

private:
    RvkGpuDevice* mDevice;
};
