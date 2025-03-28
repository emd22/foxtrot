#pragma once

#include "Core/Util.hpp"
#include "Renderer/Renderer.hpp"
#include "vulkan/vulkan_core.h"
#include <ThirdParty/vk_mem_alloc.h>
#include <Core/Types.hpp>
#include <cstring>
#include <vulkan/vulkan.h>

#include <Core/FxPanic.hpp>

#include <memory.h>

namespace vulkan {

template <typename ElementType>
class FxGPUBuffer;

template <typename ElementType>
class FxGPUBufferMapContext
{
public:
    FxGPUBufferMapContext(FxGPUBuffer<ElementType> *buffer)
        : mGPUBuffer(buffer)
    {
        this->Map();
    }

    operator void *() const
    {
        return this->MappedBuffer;
    }

    void UnMap()
    {
        if (this->MappedBuffer != nullptr) {
            vmaUnmapMemory(RendererVulkan->GPUAllocator, this->mGPUBuffer->Allocation);
        }
    }

    ~FxGPUBufferMapContext()
    {
        this->UnMap();
    }

private:
    void Map()
    {
        const VkResult status = vmaMapMemory(RendererVulkan->GPUAllocator, this->mGPUBuffer->Allocation, &this->MappedBuffer);

        if (status != VK_SUCCESS) {
            Log::Error("Could not map GPU memory to main memory! (Usage: 0x%X)", EnumToInt(this->mGPUBuffer->Usage));
            return;
        }
    }

public:
    void *MappedBuffer = nullptr;

private:
    FxGPUBuffer<ElementType> *mGPUBuffer = nullptr;
};


template <typename ElementType>
class FxGPUBuffer
{
public:
    enum class UsageType {
        Vertices = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        Indices = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    };

    const size_t ElementSize = sizeof(ElementType);

    FxGPUBuffer() = default;

    void Create(UsageType usage, uint64 element_count)
    {
        this->Size = element_count;
        this->Usage = usage;

        const uint64 buffer_size = ElementSize * element_count;

        const VkBufferCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = buffer_size,
            .usage = (VkBufferUsageFlags)EnumToInt(usage),
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
            &this->Allocation,
            nullptr
        );

        if (status != VK_SUCCESS) {
            FxPanic_("GPUBuffer", "Error allocating GPU buffer!", status);
        }

        this->Initialized = true;
    }

    /**
     * Creates a new context that automatically maps and unmaps memory at the
     * end of scope.
     *
     * To get the mapped buffer, either use `value.MappedBuffer`, or `value` when using the value as a pointer.
     */
    auto MappedContext() -> FxGPUBufferMapContext<ElementType>
    {
        return FxGPUBufferMapContext<ElementType>(this);
    }

    /**
     * Uploads data from system memory to GPU memory.
     */
    void Upload(StaticArray<ElementType> &data)
    {
        if (!this->Initialized) {
            FxPanic_("GPUBuffer", "Buffer not previously initialized on Upload()", 0);
        }

        const size_t data_size = this->ElementSize * data.Size;

        const size_t buffer_size = this->ElementSize * this->Size;

        if (data_size > buffer_size) {
            Log::Error("Upload size is larger than GPU buffer allocation size", 0);
            return;
        }

        {
            auto value = this->MappedContext();
            memcpy(value, data.Data, data_size);
        }
    }

    void Destroy()
    {
        Renderer->AddToDeletionQueue([this]() {
            if (!this->Initialized) {
                return;
            }
            Log::Debug("Called Destroy for this!", 0);

            vmaDestroyBuffer(RendererVulkan->GPUAllocator, this->Buffer, this->Allocation);

            this->Initialized = false;
        });
    }

    ~FxGPUBuffer()
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
