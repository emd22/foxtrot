#pragma once

#include "Renderer/Renderer.hpp"
#include "vulkan/vulkan_core.h"
#include <ThirdParty/vk_mem_alloc.h>
#include <Core/Types.hpp>
#include <vulkan/vulkan.h>

namespace vulkan {

template <typename ElementType>
class GPUBuffer
{
public:
    enum class UsageType {
        Vertices = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        Indices = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    };

    GPUBuffer() = default;

    void Create(UsageType usage, uint64 element_count)
    {
        this->Size = element_count;
        this->Usage = usage;

        const uint64 buffer_size = sizeof(ElementType) * element_count;

        const VkBufferCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = buffer_size,
            .usage = usage,
            .flags = 0
        };

        const VmaAllocationCreateInfo alloc_create_info = {
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
        };

        const VkResult status = vmaCreateBuffer(
            RendererVulkan->GPUAllocator,
            &create_info,
            &alloc_create_info,
            &this->Buffer,
            &this->Allocation
        );

        if (status != VK_SUCCESS) {
            Panic_("GPUBuffer", "Error allocating GPU buffer!", status);
        }

        this->Initialized = true;
    }

    void Destroy()
    {
        if (!this->Initialized) {
            return;
        }

        vmaDestroyBuffer(RendererVulkan->GPUAllocator, &this->Buffer, &this->Allocation);

        this->Initialized = false;
    }

    ~GPUBuffer()
    {
        this->Destroy();
    }


public:
    VkBuffer Buffer = nullptr;
    VmaAllocation Allocation = nullptr;

    uint64 Size = 0;

    UsageType Usage;

    bool Initialized = false;
private:
};

}; // namespace vulkan
