#include "Device.hpp"
#include "Util.hpp"

#include "vulkan/vulkan_core.h"

#include <Util/Log.hpp>
#include <climits>

#include <Backend/RenderPanic.hpp>

template <typename T, typename... Types>
void Panic(const char *fmt, T first, Types... items)
{
    RenderPanic("Device: ", fmt, first, items...);
}

template <typename T, typename... Types>
void Panic(const char *fmt, VkResult result, Types... items)
{
    RenderPanic("Device: ", fmt, result, items...);
}

static void VkTry(VkResult result, const char *message)
{
    if (result != VK_SUCCESS) {
        Panic(message, result);
    }
}

///////////////////////////////
// Queue Families
///////////////////////////////


void QueueFamilies::FindQueueFamilies(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
    uint32 family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &family_count, nullptr);

    this->RawFamilies.reserve(family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &family_count, this->RawFamilies.data());

    Log::Info("Amount of queue families: %d", family_count);

    uint32 index = 0;
    for (const auto &family : this->RawFamilies) {
        if (this->mPresentIndex != UINT_MAX && this->mGraphicsIndex != UINT_MAX) {
            break;
        }

        if (family.queueCount == 0)
            continue;

        // check for a graphics family
        {
            if ((family.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                this->mGraphicsIndex = index;
            }
        }

        // check for a present family
        {
            uint32 present_support = 0;

            VkResult status = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, index, surface, &present_support);
            if (status != VK_SUCCESS) {
                Log::Error("Could not retrieve physical device surface support", status);
                continue;
            }

            Log::Info("Present support: %d\n", present_support);
            if (present_support > 0) {
                this->mPresentIndex = index;
            }
        }

        index++;
    }
}


///////////////////////////////
// GPU Device
///////////////////////////////

bool GPUDevice::IsPhysicalDeviceSuitable()
{
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    QueueFamilies new_families;
    new_families.FindQueueFamilies(this->Physical, this->mSurface);

    vkGetPhysicalDeviceFeatures(this->Physical, &features);
    vkGetPhysicalDeviceProperties(this->Physical, &properties);

    const uint32 version = properties.apiVersion;

    if (version > VK_MAKE_VERSION(1, 2, 0) && this->mQueueFamilies.IsComplete()) {
        return true;
    }

    const auto families = new_families.GetQueueIndexList();

    Log::Info("Device not suitable: (Vk: %d.%d.%d), Graphics?: %s, Present?: %s",
        VK_VERSION_MAJOR(version),
        VK_VERSION_MINOR(version),
        VK_VERSION_PATCH(version),
        Log::YesNo(families[0] != QueueFamilies::QueueNull),
        Log::YesNo(families[1] != QueueFamilies::QueueNull)
    );

    return false;
}

void GPUDevice::QueryQueues()
{
    const auto indices = this->mQueueFamilies.GetQueueIndexList();

    if (indices.size() < 2) {
        Panic("Not enough queue families", 0);
    }

    vkGetDeviceQueue(this->Device, indices[0], 0, &this->GraphicsQueue);
    vkGetDeviceQueue(this->Device, indices[1], 0, &this->PresentQueue);
}

void GPUDevice::CreateLogicalDevice()
{
    if (this->Physical == nullptr) {
        this->PickPhysicalDevice();
    }

    if (!this->mQueueFamilies.IsComplete()) {
        this->mQueueFamilies.FindQueueFamilies(this->Physical, this->mSurface);
    }

    const auto queue_family_indices = this->mQueueFamilies.GetQueueIndexList();

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    queue_create_infos.reserve(queue_family_indices.size());

    const float queue_priority = 1.0f;

    int index = 0;

    for (const auto family_index : queue_family_indices) {
        if (family_index == QueueFamilies::QueueNull) {
            continue;
        }

        queue_create_infos.push_back(VkDeviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = family_index,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        });

        // check if our queue family index's are the same
        {
            const uint32 next_index = index + 1;
            // if the next queue family (presentation) is the same as the current one, return as we
            // do not need to create a new queue family
            if (next_index < queue_family_indices.size() && family_index == queue_family_indices[next_index]) {
                break;
            }
        }

        index++;
    }

    const VkPhysicalDeviceFeatures device_features{};
    const char *device_extensions[] = {
        // TODO: ifdef macOS/__APPLE__ here
        "VK_KHR_portability_subset",
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
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
    };

    const VkResult status = vkCreateDevice(this->Physical, &create_info, nullptr, &this->Device);

    if (status != VK_SUCCESS) {
        Panic("Could not create logical device", status);
    }

    this->QueryQueues();
}

GPUDevice::GPUDevice(VkInstance instance, VkSurfaceKHR surface)
    : mInstance(instance), mSurface(surface)
{
    this->PickPhysicalDevice();

    this->mQueueFamilies.FindQueueFamilies(this->Physical, surface);

}

VkSurfaceFormatKHR GPUDevice::GetBestSurfaceFormat()
{
    uint32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(this->Physical, this->mSurface, &format_count, nullptr);

    std::vector<VkSurfaceFormatKHR> surface_formats(format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(this->Physical, this->mSurface, &format_count, surface_formats.data());

    for (const auto &format : surface_formats) {
        if (format.format == VK_FORMAT_B8G8R8_SRGB) {
            return format;
        }
    }

    return surface_formats[0];
}

void GPUDevice::PickPhysicalDevice()
{
    std::vector<VkPhysicalDevice> physical_devices;

    // enumerate physical devices
    {
        const auto enum_err = "Could not enumerate physical devices";

        uint32 device_count;
        VkTry(vkEnumeratePhysicalDevices(this->mInstance, &device_count, nullptr), enum_err);

        if (device_count == 0) {
            Panic("No usable physical devices found (no vulkan support!)", 0);
        }

        physical_devices.reserve(device_count);

        VkTry(vkEnumeratePhysicalDevices(this->mInstance, &device_count, physical_devices.data()), enum_err);
    }

    for (VkPhysicalDevice &device : physical_devices) {
        if (IsPhysicalDeviceSuitable()) {
            this->Physical = device;
            break;
        }
    }
    if (this->Physical == nullptr) {
        Panic("Could not find a suitable physical device!", 0);
    }
}
