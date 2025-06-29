#include "RvkDevice.hpp"
#include "Core/Defines.hpp"
#include "Core/FxSizedArray.hpp"

#include <Core/Log.hpp>

#include <Core/FxPanic.hpp>

#include <vulkan/vulkan.h>

FX_SET_MODULE_NAME("Device")

///////////////////////////////
// Queue Families
///////////////////////////////

void RvkQueueFamilies::FindGraphicsFamily(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    for (int32 index = 0; index < RawFamilies.Size; index++) {
        VkQueueFamilyProperties &family_props = RawFamilies[index];

        // Presentation families will usually always have graphics support. Best to choose the most
        // capable here as our main family.
        if ((family_props.queueFlags & VK_QUEUE_GRAPHICS_BIT) && FamilyHasPresentSupport(device, surface, index)) {
            mGraphicsIndex = index;
            mPresentIndex = index;
        }
    }
}

void RvkQueueFamilies::FindTransferFamily(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    for (int32 index = 0; index < RawFamilies.Size; index++) {
        VkQueueFamilyProperties &family_props = RawFamilies[index];

        // We want a separate queue here for
        if ((family_props.queueFlags & VK_QUEUE_TRANSFER_BIT) && index != mGraphicsIndex) {
            mTransferIndex = index;
        }
    }
}

bool RvkQueueFamilies::FamilyHasPresentSupport(VkPhysicalDevice device, VkSurfaceKHR surface, uint32 family_index)
{
    uint32 support = 0;

    VkResult status = vkGetPhysicalDeviceSurfaceSupportKHR(device, family_index, surface, &support);

    if (status != VK_SUCCESS) {
        Log::Error("Could not query present support for family at index %d", family_index);
        return false;
    }

    return support > 0;
}

void RvkQueueFamilies::FindQueueFamilies(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32 family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &family_count, nullptr);

    RawFamilies.InitSize(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &family_count, RawFamilies.Data);

    Log::Info("Amount of queue families: %d", family_count);


    FindGraphicsFamily(physical_device, surface);
    FindTransferFamily(physical_device, surface);
}


///////////////////////////////
// GPU Device
///////////////////////////////

bool RvkGpuDevice::IsPhysicalDeviceSuitable(VkPhysicalDevice &physical)
{
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    RvkQueueFamilies new_families;
    new_families.FindQueueFamilies(physical, mSurface);

    vkGetPhysicalDeviceFeatures(physical, &features);
    vkGetPhysicalDeviceProperties(physical, &properties);

    const uint32 version = properties.apiVersion;

    if (version >= VK_MAKE_VERSION(1, 3, 0) && new_families.IsComplete()) {
        Log::Info("Device suitable: (Supports Vk: %d.%d.%d), Graphics?: %s, Present?: %s, Xfer?: %s, IsComplete?: %s",
            VK_VERSION_MAJOR(version),
            VK_VERSION_MINOR(version),
            VK_VERSION_PATCH(version),
            Log::YesNo(new_families.GetGraphicsFamily() != RvkQueueFamilies::QueueNull),
            Log::YesNo(new_families.GetPresentFamily() != RvkQueueFamilies::QueueNull),
            Log::YesNo(new_families.GetTransferFamily() != RvkQueueFamilies::QueueNull),
            Log::YesNo(new_families.IsComplete())
        );
        return true;
    }


    Log::Info("Device not suitable: (Supports Vk: %d.%d.%d), Graphics?: %s, Present?: %s, Xfer?: %s, IsComplete?: %s",
        VK_VERSION_MAJOR(version),
        VK_VERSION_MINOR(version),
        VK_VERSION_PATCH(version),
        Log::YesNo(new_families.GetGraphicsFamily() != RvkQueueFamilies::QueueNull),
        Log::YesNo(new_families.GetPresentFamily() != RvkQueueFamilies::QueueNull),
        Log::YesNo(new_families.GetTransferFamily() != RvkQueueFamilies::QueueNull),
        Log::YesNo(new_families.IsComplete())
    );

    return false;
}

void RvkGpuDevice::QueryQueues()
{
    vkGetDeviceQueue(Device, mQueueFamilies.GetGraphicsFamily(), 0, &GraphicsQueue);
    vkGetDeviceQueue(Device, mQueueFamilies.GetPresentFamily(), 0, &PresentQueue);

    vkGetDeviceQueue(Device, mQueueFamilies.GetTransferFamily(), 0, &TransferQueue);
}

void RvkGpuDevice::CreateLogicalDevice()
{
    if (Physical == nullptr) {
        PickPhysicalDevice();
    }

    if (!mQueueFamilies.IsComplete()) {
        mQueueFamilies.FindQueueFamilies(Physical, mSurface);
    }

    const auto queue_family_indices = mQueueFamilies.GetQueueIndexList();

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    queue_create_infos.reserve(queue_family_indices.size());

    const float graphics_priorities[] = { 1.0f };
    float transfer_priorities[] = { 1.0f };

    queue_create_infos.push_back(VkDeviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = mQueueFamilies.GetGraphicsFamily(),
        .queueCount = 1,
        .pQueuePriorities = graphics_priorities,
    });

    queue_create_infos.push_back(VkDeviceQueueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = mQueueFamilies.GetTransferFamily(),
        .queueCount = 1,
        .pQueuePriorities = transfer_priorities,
    });

    const VkPhysicalDeviceFeatures device_features{};

    const char *device_extensions[] = {
    #ifdef FX_PLATFORM_MACOS
        "VK_KHR_portability_subset",
    #endif
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    // lol Dr. Features
    VkPhysicalDeviceDynamicRenderingFeaturesKHR dr_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .dynamicRendering = true,
        .pNext = nullptr
    };

    const VkDeviceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queue_create_infos.data(),
        .queueCreateInfoCount = (uint32)queue_create_infos.size(),
        .pEnabledFeatures = &device_features,

        // device specific extensions
        .enabledExtensionCount = sizeof(device_extensions) / sizeof(device_extensions[0]),
        .ppEnabledExtensionNames = device_extensions,

        // device specific layers (not used in modern vulkan)
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,

        .pNext = &dr_features
    };

    const VkResult status = vkCreateDevice(Physical, &create_info, nullptr, &Device);

    if (status != VK_SUCCESS) {
        FxModulePanic("Could not create logical device", status);
    }

    QueryQueues();
}

void RvkGpuDevice::Create(VkInstance instance, VkSurfaceKHR surface)
{
    mInstance = instance;
    mSurface = surface;

    PickPhysicalDevice();

    mQueueFamilies.FindQueueFamilies(Physical, surface);

    CreateLogicalDevice();

    Log::Info("Device Queue Families: \n\tPresent: %d\n\tGraphics: %d\n\tTransfer: %d",
        mQueueFamilies.GetPresentFamily(), mQueueFamilies.GetGraphicsFamily(), mQueueFamilies.GetTransferFamily());
}

void RvkGpuDevice::Destroy()
{
    vkDestroyDevice(Device, nullptr);
}

VkSurfaceFormatKHR RvkGpuDevice::GetBestSurfaceFormat()
{
    uint32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(Physical, mSurface, &format_count, nullptr);

    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(Physical, mSurface, &format_count, surface_formats.data());

    for (const auto &format : surface_formats) {
        if (format.format == VK_FORMAT_B8G8R8_SRGB) {
            return format;
        }
    }

    return surface_formats[0];
}

void RvkGpuDevice::PickPhysicalDevice()
{
    // enumerate physical devices
    //
    uint32 device_count;
    VkTry(vkEnumeratePhysicalDevices(mInstance, &device_count, nullptr), "Could not enumerate physical devices");

    if (device_count == 0) {
        FxModulePanic("No usable physical devices found (no vulkan support!)", 0);
    }

    FxSizedArray<VkPhysicalDevice> physical_devices;
    physical_devices.InitSize(device_count);

    VkTry(vkEnumeratePhysicalDevices(mInstance, &device_count, physical_devices.Data), "Could not enumerate physical devices");

    for (VkPhysicalDevice &device : physical_devices) {
        if (IsPhysicalDeviceSuitable(device)) {
            Physical = device;
            break;
        }
    }
    if (Physical == nullptr) {
        FxModulePanic("Could not find a suitable physical device!", 0);
    }
}

void RvkGpuDevice::WaitForIdle()
{
    vkDeviceWaitIdle(Device);
}
