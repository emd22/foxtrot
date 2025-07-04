#pragma once

#include "Core/FxSizedArray.hpp"
#include <Core/Types.hpp>

#include <cstdint>
#include <vector>

#include <vulkan/vulkan.h>

class RvkQueueFamilies {
public:
    static const uint32 QueueNull = UINT32_MAX;

    void FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

    /// Checks if the main graphics and presentation queues have been queried
    bool IsComplete()
    {
        return (mGraphicsIndex != QueueNull
             && mPresentIndex != QueueNull
             && mTransferIndex != QueueNull);
    }

    /// Retrieves the available queue indices
    std::vector<uint32> GetQueueIndexList() const
    {
        return std::vector<uint32>({
            mGraphicsIndex,
            mPresentIndex,

            mTransferIndex,
        });
    }

    uint32 GetGraphicsFamily() { return mGraphicsIndex; }
    uint32 GetPresentFamily() { return mPresentIndex; }
    uint32 GetTransferFamily() { return mTransferIndex; }

private:
    void FindGraphicsFamily(VkPhysicalDevice device, VkSurfaceKHR surface);
    void FindTransferFamily(VkPhysicalDevice device, VkSurfaceKHR surface);

    bool FamilyHasPresentSupport(VkPhysicalDevice device, VkSurfaceKHR surface, uint32 family_index);

public:
    FxSizedArray<VkQueueFamilyProperties> RawFamilies;
private:
    uint32 mGraphicsIndex = QueueNull;
    uint32 mPresentIndex = QueueNull;
    uint32 mTransferIndex = QueueNull;
};

class RvkGpuDevice {
public:
    RvkGpuDevice() = default;
    RvkGpuDevice(VkInstance instance, VkSurfaceKHR surface)
    {
        Create(instance, surface);
    }

    void Create(VkInstance instance, VkSurfaceKHR surface);
    void Destroy();

    void PickPhysicalDevice();
    void CreateLogicalDevice();

    void WaitForIdle();

    VkSurfaceFormatKHR GetBestSurfaceFormat();

    operator VkDevice () const { return Device; }
    operator VkPhysicalDevice () const { return Physical; }

private:
    bool IsPhysicalDeviceSuitable(VkPhysicalDevice &device);
    void QueryQueues();

public:
    VkPhysicalDevice Physical = nullptr;
    VkDevice Device = nullptr;

    RvkQueueFamilies mQueueFamilies;

    VkQueue GraphicsQueue = nullptr;
    VkQueue TransferQueue = nullptr;
    VkQueue PresentQueue = nullptr;

private:
    VkInstance mInstance;
    VkSurfaceKHR mSurface;
};
