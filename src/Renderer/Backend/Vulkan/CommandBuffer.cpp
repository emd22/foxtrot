#include "CommandBuffer.hpp"
#include "../RenderPanic.hpp"
#include "Device.hpp"
#include "Renderer/Renderer.hpp"
#include "vulkan/vulkan_core.h"

namespace vulkan {

AR_SET_MODULE_NAME("CommandBuffer")

void CommandBuffer::Create(CommandPool *pool)
{
    this->mCommandPool = pool;

    const VkCommandBufferAllocateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool->CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    this->mDevice = RendererVulkan->GetDevice();

    const VkResult status = vkAllocateCommandBuffers(this->mDevice->Device, &buffer_info, &this->CommandBuffer);
    if (status != VK_SUCCESS) {
        Panic("Could not allocate command buffer", status);
    }


    this->mInitialized = true;
}

inline void CommandBuffer::CheckInitialized() const
{
    if (!this->IsInitialized()) {
        Panic("Command buffer has not been initialized!", 0);
    }
}

void CommandBuffer::Record()
{
    this->CheckInitialized();

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr
    };

    const VkResult status = vkBeginCommandBuffer(this->CommandBuffer, &begin_info);
    if (status != VK_SUCCESS) {
        Panic("Failed to begin recording command buffer", status);
    }
}

void CommandBuffer::Reset()
{
    this->CheckInitialized();

    vkResetCommandBuffer(this->CommandBuffer, 0);
}

void CommandBuffer::End()
{
    this->CheckInitialized();

    const VkResult status = vkEndCommandBuffer(this->CommandBuffer);
    if (status != VK_SUCCESS) {
        Panic("Failed to create command buffer!", status);
    }
}

void CommandBuffer::Destroy()
{
    vkFreeCommandBuffers(this->mDevice->Device, this->mCommandPool->CommandPool, 1, &this->CommandBuffer);
}


}; // namespace vulkan
