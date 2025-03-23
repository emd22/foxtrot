#pragma once

#include <Core/Types.hpp>

#include "vulkan/vulkan_core.h"
#include <cstdint>
#include <vulkan/vulkan.h>

#include <vector>

class QueueFamilies {
public:
    static const uint32 QueueNull = UINT32_MAX;

    void FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

    /// Checks if the main graphics and presentation queues have been queried
    bool IsComplete()
    {
        return (this->mGraphicsIndex != QueueNull
             && this->mPresentIndex != QueueNull);
    }

    /// Retrieves the available queue indices
    std::vector<uint32> GetQueueIndexList() const
    {
        return std::vector<uint32>({
            this->mGraphicsIndex,
            this->mPresentIndex,
        });
    }

    /// Checks if the graphics queue is also the presentation queue
    bool IsGraphicsAlsoPresent() const
    {
        return (this->mGraphicsIndex == this->mPresentIndex);
    }

public:
    std::vector<VkQueueFamilyProperties> RawFamilies;
private:
    uint32 mGraphicsIndex = QueueNull;
    uint32 mPresentIndex = QueueNull;
};

class GPUDevice {
public:
    GPUDevice(VkInstance instance, VkSurfaceKHR surface);

    void PickPhysicalDevice();
    void CreateLogicalDevice();

    VkSurfaceFormatKHR GetBestSurfaceFormat();

private:
    bool IsPhysicalDeviceSuitable();
    void QueryQueues();

public:
    VkPhysicalDevice Physical = nullptr;
    VkDevice Device = nullptr;

    QueueFamilies mQueueFamilies;

    VkQueue GraphicsQueue = nullptr;
    VkQueue PresentQueue = nullptr;

private:
    VkInstance mInstance;
    VkSurfaceKHR mSurface;
};
