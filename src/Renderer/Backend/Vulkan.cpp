#include "Vulkan.hpp"

#include <Core/Defines.hpp>
#include <Core/Types.hpp>

#include <Core/Log.hpp>

#include "Renderer/Backend/Vulkan/Fence.hpp"
#include "Renderer/Backend/Vulkan/Pipeline.hpp"
#include "Renderer/Backend/Vulkan/Semaphore.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/CommandBuffer.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <vulkan/vulkan.h>

#include "Vulkan/Util.hpp"
#include "RenderPanic.hpp"
#include "vulkan/vulkan_core.h"

#include <ThirdParty/vk_mem_alloc.h>

#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL.h>

#define AR_VULKAN_DEBUG 1

namespace vulkan {

using ExtensionNames = VkRenderBackend::ExtensionNames;
using ExtensionList = VkRenderBackend::ExtensionList;

AR_SET_MODULE_NAME("Vulkan")

ExtensionNames VkRenderBackend::CheckExtensionsAvailable(ExtensionNames &requested_extensions)
{
    if (this->mAvailableExtensions.IsEmpty()) {
        this->QueryInstanceExtensions();
    }

    std::vector<const char *> missing_extensions;

    for (const char *requested_name : requested_extensions) {
        bool found_extension = false;
        for (const auto &extension : this->mAvailableExtensions) {
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

StaticArray<VkLayerProperties> VkRenderBackend::GetAvailableValidationLayers()
{
    uint32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    StaticArray<VkLayerProperties> validation_layers;
    validation_layers.InitSize(layer_count);

    vkEnumerateInstanceLayerProperties(&layer_count, validation_layers.Data);

    return validation_layers;
}

VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance);

void VkRenderBackend::Init(Vec2i window_size)
{
    this->InitVulkan();
    this->CreateSurfaceFromWindow();

    this->mDevice.Create(this->mInstance, this->mWindowSurface);

    this->InitGPUAllocator();
    this->Swapchain.Init(window_size, this->mWindowSurface, &this->mDevice);

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

    if ((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) {
        Log::Error(fmt, message);
    }
    else if ((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)) {
        Log::Warning(fmt, message);
    }
    else if ((message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)) {
        Log::Info(fmt, message);
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

void VkRenderBackend::InitGPUAllocator()
{
    const GPUDevice *device = this->GetDevice();

    const VmaAllocatorCreateInfo create_info = {
        .physicalDevice = device->Physical,
        .device = device->Device,
        .instance = this->mInstance
    };

    const VkResult status = vmaCreateAllocator(&create_info, &this->GPUAllocator);
    if (status != VK_SUCCESS) {
        Panic("Could not create VMA allocator!", status);
    }
}

void VkRenderBackend::DestroyGPUAllocator()
{
    vmaDestroyAllocator(this->GPUAllocator);
}

ExtensionNames VkRenderBackend::MakeInstanceExtensionList(ExtensionNames &user_requested_extensions)
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

ExtensionList &VkRenderBackend::QueryInstanceExtensions(bool invalidate_previous)
{
    Log::Debug("Query Extensions", 0);


    if (this->mAvailableExtensions.IsNotEmpty()) {
        if (invalidate_previous) {
            this->mAvailableExtensions.Free();
        }
        else {
            return this->mAvailableExtensions;
        }
    }


    // Get the count of the current extensions
    uint32_t extension_count = 0;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Could not query instance extensions!");
    }

    std::cout << "Ext count: " << extension_count << "\n";

    // resize to create the items in the vector
    this->mAvailableExtensions.InitSize(extension_count);

    // Get the available instance extensions
    result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, this->mAvailableExtensions.Data);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Could not query instance extensions!");
    }

    std::cout << "=== Available Instance Extensions (" << extension_count << ") ===\n";

    for (uint32_t i = 0; i < extension_count; ++i) {
        const VkExtensionProperties& extension = mAvailableExtensions[i];
        std::cout << extension.extensionName << " : " << extension.specVersion << "\n";
    }

    return this->mAvailableExtensions;
}

FrameResult VkRenderBackend::BeginFrame(GraphicsPipeline &pipeline)
{
    FrameData *frame = this->GetFrame();

    frame->InFlight.WaitFor();

    FrameResult result = this->GetNextSwapchainImage(frame);
    if (result != FrameResult::Success) {
        return result;
    }

    frame->InFlight.Reset();

    frame->CommandBuffer.Reset();
    frame->CommandBuffer.Record();

    pipeline.RenderPass.Begin();
    pipeline.Bind(frame->CommandBuffer);

    const int32 width = this->Swapchain.Extent.Width();
    const int32 height = this->Swapchain.Extent.Height();

    const VkViewport viewport = {
        .x = 0, .y = 0,
        .width = (float32)width,
        .height = (float32)height,
        .minDepth = 0.0,
        .maxDepth = 1.0,
    };

    vkCmdSetViewport(frame->CommandBuffer.CommandBuffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = { .x = 0, .y = 0 },
        .extent = { .width = (uint32)width, .height = (uint32)height }
    };

    vkCmdSetScissor(frame->CommandBuffer.CommandBuffer, 0, 1, &scissor);

    return FrameResult::Success;
}

void VkRenderBackend::SubmitFrame()
{
    FrameData *frame = this->GetFrame();

    const VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame->ImageAvailable.Semaphore,
        .pWaitDstStageMask = wait_stages,
        // command buffers
        .commandBufferCount = 1,
        .pCommandBuffers = &frame->CommandBuffer.CommandBuffer,
        // signal semaphores
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &frame->RenderFinished.Semaphore
    };

    VkTry(
        vkQueueSubmit(this->GetDevice()->GraphicsQueue, 1, &submit_info, frame->InFlight.Fence),
        "Error submitting draw buffer"
    );
}

void VkRenderBackend::PresentFrame()
{
    FrameData *frame = this->GetFrame();

    if (this->Swapchain.Initialized != true) {
        Panic("Swapchain not initialized!", 0);
    }

    const VkSwapchainKHR swapchains[] = {
        this->Swapchain.GetSwapchain(),
    };

    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame->RenderFinished.Semaphore,

        .swapchainCount = 1,
        .pSwapchains = swapchains,

        .pImageIndices = &this->mImageIndex,

        .pResults = nullptr,
    };

    const VkResult status = vkQueuePresentKHR(this->GetDevice()->PresentQueue, &present_info);

    if (status == VK_SUCCESS) { }
    else if (status == VK_ERROR_OUT_OF_DATE_KHR || status == VK_SUBOPTIMAL_KHR) {
        // Swapchain.Rebuild()..
    }
    else {
        Log::Error("Error submitting present queue", status);
    }
}

void VkRenderBackend::FinishFrame(GraphicsPipeline &pipeline)
{
    pipeline.RenderPass.End();

    this->GetFrame()->CommandBuffer.End();

    this->SubmitFrame();
    this->PresentFrame();

    this->ProcessDeletionQueue();

    this->mInternalFrameCounter++;

    // better to do one division per frame as opposed to one every GetFrameNumber()
    this->mFrameNumber = (mInternalFrameCounter) % this->FramesInFlight;
}

FrameResult VkRenderBackend::GetNextSwapchainImage(FrameData *frame)
{
    const uint64 timeout = UINT64_MAX; // TODO: change this value and handle AcquireNextImage errors correctly

    const VkResult result = vkAcquireNextImageKHR(
        this->GetDevice()->Device,
        this->Swapchain.GetSwapchain(),
        timeout,
        frame->ImageAvailable.Semaphore,
        nullptr,
        &this->mImageIndex
    );

    if (result == VK_SUCCESS) {
        return FrameResult::Success;
    }
    else if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Swapchain.Rebuild()..
        return FrameResult::GraphicsOutOfDate;
    }
    else {
        Log::Error("Error getting next swapchain image!", result);
    }

    return FrameResult::RenderError;
}

void VkRenderBackend::CreateSurfaceFromWindow()
{
    if (this->mWindow == nullptr) {
        Panic("No window attached! use VkRenderBackend::SelectWindow()", 0);
    }

    bool success = SDL_Vulkan_CreateSurface(this->mWindow->GetWindow(), this->mInstance, nullptr, &this->mWindowSurface);

    if (!success) {
        Panic("Could not attach Vulkan instance to window! (SDL err: %s)", SDL_GetError());
    }
}

void VkRenderBackend::Destroy()
{
    this->GetDevice()->WaitForIdle();

    while (!this->mDeletionQueue.empty()) {
        this->ProcessDeletionQueue(true);
        // insert a small delay to avoid the processor spinning out while
        // waiting for an object. this allows handing the core off to other threads.
        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
    }

    this->DestroyGPUAllocator();

    this->Swapchain.Destroy();
    this->DestroyFrames();

    if (this->mWindowSurface) {
        vkDestroySurfaceKHR(this->mInstance, this->mWindowSurface, nullptr);
    }

    this->GetDevice()->Destroy();

    DestroyDebugMessenger(this->mInstance, this->mDebugMessenger);

    vkDestroyInstance(this->mInstance, nullptr);

    this->Initialized = false;
}

FrameData *VkRenderBackend::GetFrame()
{
    return &this->Frames[this->GetFrameNumber()];
}

}; // namespace vulkan
