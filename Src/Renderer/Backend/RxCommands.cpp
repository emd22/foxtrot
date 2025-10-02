#include "RxCommands.hpp"

#include "RxDevice.hpp"

#include <Core/FxPanic.hpp>
#include <FxEngine.hpp>

FX_SET_MODULE_NAME("RxCommandBuffer")

void RxCommandBuffer::Create(RxCommandPool* pool)
{
    mCommandPool = pool;

    const VkCommandBufferAllocateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool->CommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    mDevice = gRenderer->GetDevice();

    const VkResult status = vkAllocateCommandBuffers(mDevice->Device, &buffer_info, &CommandBuffer);
    if (status != VK_SUCCESS) {
        FxModulePanicVulkan("Could not allocate command buffer", status);
    }

    FxLogDebug("Creating Command buffer 0x{:p} from queue family {:d}", reinterpret_cast<void*>(CommandBuffer),
               pool->QueueFamilyIndex);

    mInitialized = true;
}

inline void RxCommandBuffer::CheckInitialized() const
{
    if (!IsInitialized()) {
        FxModulePanic("Command buffer has not been initialized!", 0);
    }
}

void RxCommandBuffer::Record(VkCommandBufferUsageFlags usage_flags)
{
    CheckInitialized();

    const VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = nullptr,
    };

    const VkResult status = vkBeginCommandBuffer(CommandBuffer, &begin_info);
    if (status != VK_SUCCESS) {
        FxModulePanicVulkan("Failed to begin recording command buffer", status);
    }
}

void RxCommandBuffer::Reset()
{
    CheckInitialized();

    vkResetCommandBuffer(CommandBuffer, 0);
}

void RxCommandBuffer::End()
{
    CheckInitialized();

    const VkResult status = vkEndCommandBuffer(CommandBuffer);
    if (status != VK_SUCCESS) {
        FxModulePanicVulkan("Failed to create command buffer!", status);
    }
}

void RxCommandBuffer::Destroy() { vkFreeCommandBuffers(mDevice->Device, mCommandPool->CommandPool, 1, &CommandBuffer); }
