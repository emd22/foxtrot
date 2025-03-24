#include "Vulkan.hpp"

#include <Core/Defines.hpp>
#include <Core/Types.hpp>

#include <Util/Log.hpp>

#include "Renderer/Backend/Vulkan/Fence.hpp"
#include "Renderer/Backend/Vulkan/Semaphore.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/CommandBuffer.hpp"

#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>

#include "Vulkan/Util.hpp"
#include "RenderPanic.hpp"

#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL.h>

#define AR_VULKAN_DEBUG 1

namespace vulkan {

using ExtensionNames = VkRenderBackend::ExtensionNames;
using ExtensionList = VkRenderBackend::ExtensionList;

AR_SET_MODULE_NAME("Vulkan")

ExtensionNames VkRenderBackend::CheckExtensionsAvailable(ExtensionNames requested_extensions)
{
    if (this->mAvailableExtensions.empty()) {
        this->QueryInstanceExtensions();
    }

    std::vector<const char *> missing_extensions;

    for (const char *requested_name : requested_extensions) {
        bool found_extension = false;
        for (const auto &extension : mAvailableExtensions) {
            if (!strncmp(extension.extensionName, requested_name, 256)) {
                found_extension = true;
                break;
            }
        }

        if (!found_extension) {
            missing_extensions.push_back(requested_name);
        }
    }

    return missing_extensions;
}

std::vector<VkLayerProperties> VkRenderBackend::GetAvailableValidationLayers()
{
    uint32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> validation_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, validation_layers.data());

    return validation_layers;
}

VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance);

void VkRenderBackend::Init(Vec2i window_size)
{
    this->InitVulkan();
    this->CreateSurfaceFromWindow();

    this->mDevice.Create(this->mInstance, this->mWindowSurface);
    this->Swapchain.Init(window_size, this->mWindowSurface, this->mDevice);

    this->InitFrames();

    this->Initialized = true;
}

void VkRenderBackend::InitFrames()
{
    this->Frames.InitSize(this->FramesInFlight);

    const uint32 graphics_family = this->GetDevice()->mQueueFamilies.GetGraphicsFamily();

    GPUDevice *device = this->GetDevice();

    for (int i = 0; i < this->Frames.Size; i++) {
        this->Frames.Data[i].CommandPool.Create(device, graphics_family);
        this->Frames.Data[i].CommandBuffer.Create(&this->Frames.Data[i].CommandPool);

        this->Frames.Data[i].Create(device);
    }
}

void VkRenderBackend::DestroyFrames()
{
    for (auto &frame : this->Frames) {
        frame.CommandBuffer.Destroy();
        frame.CommandPool.Destroy();

        frame.Destroy();
    }

    this->Frames.Free();
}

void VkRenderBackend::InitVulkan()
{
    const char* app_name = "Rocket";
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name;
    app_info.pEngineName = app_name;
    app_info.apiVersion = VK_MAKE_VERSION(1, 3, 261);

    ExtensionNames requested_extensions = {
        VK_EXT_LAYER_SETTINGS_EXTENSION_NAME,
    };

    ExtensionNames all_extensions = MakeInstanceExtensionList(requested_extensions);

#ifdef AR_VULKAN_DEBUG
    all_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    all_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

    std::cout << "Requested to load " << all_extensions.size() << " extensions...\n";

    for (const auto& extension : all_extensions) {
        Log::Debug("Ext: %s", extension);
    }

    ExtensionNames missing_extensions = CheckExtensionsAvailable(all_extensions);

    if (!missing_extensions.empty()) {
        std::cerr << "MISSING: ";
        for (size_t i = 0; i < missing_extensions.size(); ++i) {
            std::cerr << missing_extensions[i];
            if (i < missing_extensions.size() - 1) {
                std::cerr << ", ";
            }
        }

        Panic("Missing required instance extensions", 0);
    }

    auto validation_layers = GetAvailableValidationLayers();
    for (auto &layer : validation_layers) {
        Log::Debug("Layer: %s", layer.layerName);
    }

    std::vector<const char*> requested_validation_layers = {
        "VK_LAYER_KHRONOS_validation",
        // "VK_LAYER_KHRONOS_shader_object",
    };

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.ppEnabledExtensionNames = all_extensions.data();
    instance_info.enabledExtensionCount = static_cast<uint32_t>(all_extensions.size());
    instance_info.ppEnabledLayerNames = requested_validation_layers.data();
    instance_info.enabledLayerCount = static_cast<uint32_t>(requested_validation_layers.size());
    instance_info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    VkResult result = vkCreateInstance(&instance_info, nullptr, &mInstance);

    if (result != VK_SUCCESS) {
        Panic("Could not create vulkan instance!", result);
    }

#ifdef AR_VULKAN_DEBUG
    mDebugMessenger = CreateDebugMessenger(mInstance);
    if (!mDebugMessenger) {
        Panic("Could not create debug messenger", 0);
    }
#endif

    this->Initialized = true;
}

uint32 DebugMessageCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, uint32 type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data
) {
    const char * message = callback_data->pMessage;
    const char *fmt = "VkValidator: %s";

    if (!(message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)) {
        Log::Info(fmt, message);
    }
    else if (!(message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)) {
        Log::Warning(fmt, message);
    }
    else if (!(message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) {
        Log::Error(fmt, message);
    }
    else {
        Log::Debug(fmt, message);
    }

    return 0;
}

template<typename FuncProt>
FuncProt GetExtensionFuncVk(VkInstance instance, const char *name)
{
    PFN_vkVoidFunction raw_ptr = vkGetInstanceProcAddr(instance, name);

    if (raw_ptr != nullptr) {
        return (FuncProt)raw_ptr;
    }

    Log::Error("Could not load extension function '%s'", name);
    return nullptr;
}

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    using FuncSig = VkResult (*)(VkInstance, const VkDebugUtilsMessengerCreateInfoEXT *, const VkAllocationCallbacks *, VkDebugUtilsMessengerEXT *);

    const auto returned_function = GetExtensionFuncVk<FuncSig>(instance, "vkCreateDebugUtilsMessengerEXT");
    return returned_function(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks *pAllocator) {
    using FuncSig = void (*)(VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks *);

    const auto returned_function = GetExtensionFuncVk<FuncSig>(instance, "vkDestroyDebugUtilsMessengerEXT");
    return returned_function(instance, messenger, pAllocator);
}


VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance)
{
    const auto severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

    const auto message_type = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;


    VkDebugUtilsMessengerCreateInfoEXT create_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = severity,
        .messageType = message_type,
        .pfnUserCallback = DebugMessageCallback
    };

    VkDebugUtilsMessengerEXT messenger;

    const auto status = CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr, &messenger);
    if (status != VK_SUCCESS) {
        Log::Error("Could not create debug messenger! (err: %s)", VulkanUtil::ResultToStr(status));
        return nullptr;
    }

    return messenger;
}

void DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger) {
    if (messenger == nullptr) {
        return;
    }

    DestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
}

ExtensionNames VkRenderBackend::MakeInstanceExtensionList(ExtensionNames user_requested_extensions)
{
    uint32 required_extension_count = 0;
    const char * const * required_extensions = SDL_Vulkan_GetInstanceExtensions(&required_extension_count);

    QueryInstanceExtensions();

    const uint32 total_extensions_size = user_requested_extensions.size() + required_extension_count;
    ExtensionNames total_extensions;
    total_extensions.reserve(total_extensions_size);

    // append the user requested extensions
    total_extensions.insert(total_extensions.begin(), user_requested_extensions.begin(), user_requested_extensions.end());

    for (int32 i = 0; i < required_extension_count; i++) {
        total_extensions.push_back(required_extensions[i]);
    }

    return total_extensions;
}

ExtensionList VkRenderBackend::QueryInstanceExtensions(bool invalidate_previous)
{
    std::cout << "Query extensions\n";

    if (!mAvailableExtensions.empty()) {
        if (!invalidate_previous) {
            return mAvailableExtensions;
        }
        mAvailableExtensions.clear();
    }

    // Get the count of the current extensions
    uint32_t extension_count = 0;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Could not query instance extensions!");
    }

    std::cout << "Ext count: " << extension_count << "\n";

    // resize to create the items in the vector
    mAvailableExtensions.resize(extension_count);

    // Get the available instance extensions
    result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, mAvailableExtensions.data());
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Could not query instance extensions!");
    }

    std::cout << "=== Available Instance Extensions (" << extension_count << ") ===\n";

    for (uint32_t i = 0; i < extension_count; ++i) {
        const VkExtensionProperties& extension = mAvailableExtensions[i];
        std::cout << extension.extensionName << " : " << extension.specVersion << "\n";
    }

    return mAvailableExtensions;
}

void VkRenderBackend::CreateSurfaceFromWindow()
{
    if (this->mWindow == nullptr) {
        Panic("No window attached! use VkRenderBackend::SelectWindow()", 0);
    }

    bool success = SDL_Vulkan_CreateSurface(this->mWindow, this->mInstance, nullptr, &this->mWindowSurface);
    if (!success) {
        Panic("Could not attach Vulkan instance to window! (SDL err: %s)", SDL_GetError());
    }
}

void VkRenderBackend::Destroy()
{
    DestroyDebugMessenger(this->mInstance, this->mDebugMessenger);

    if (this->mWindowSurface) {
        vkDestroySurfaceKHR(this->mInstance, this->mWindowSurface, nullptr);
    }

    vkDestroyInstance(this->mInstance, nullptr);

    this->Initialized = false;
}

FrameData *VkRenderBackend::GetFrame()
{
    return &this->Frames[this->GetFrameNumber()];
}

}; // namespace vulkan
