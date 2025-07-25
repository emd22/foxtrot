#include "FxRenderBackend.hpp"
#include "Constants.hpp"

#include <Core/Defines.hpp>
#include <Core/Types.hpp>
#include <Core/FxPanic.hpp>
#include <Core/Log.hpp>

#include "Backend/RvkSynchro.hpp"
#include "Backend/RvkPipeline.hpp"
#include "Backend/RvkCommands.hpp"
#include "Backend/RvkUtil.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "FxDeferred.hpp"


#include <ThirdParty/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL.h>

#define FX_VULKAN_DEBUG 1

using ExtensionNames = FxRenderBackend::ExtensionNames;
using ExtensionList = FxRenderBackend::ExtensionList;

FX_SET_MODULE_NAME("RenderBackend")

ExtensionNames FxRenderBackend::CheckExtensionsAvailable(ExtensionNames &requested_extensions)
{
    if (mAvailableExtensions.IsEmpty()) {
        QueryInstanceExtensions();
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

FxSizedArray<VkLayerProperties> FxRenderBackend::GetAvailableValidationLayers()
{
    uint32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    FxSizedArray<VkLayerProperties> validation_layers;
    validation_layers.InitSize(layer_count);

    vkEnumerateInstanceLayerProperties(&layer_count, validation_layers.Data);

    return validation_layers;
}

VkDebugUtilsMessengerEXT CreateDebugMessenger(VkInstance instance);

void FxRenderBackend::Init(FxVec2u window_size)
{
    InitVulkan();
    CreateSurfaceFromWindow();

    mDevice.Create(mInstance, mWindowSurface);

    InitGPUAllocator();
    Swapchain.Init(window_size, mWindowSurface, &mDevice);

    InitFrames();
    InitUploadContext();

    Initialized = true;
}

void FxRenderBackend::InitUploadContext()
{
    UploadContext.CommandPool.Create(GetDevice(), GetDevice()->mQueueFamilies.GetTransferFamily());
    UploadContext.CommandBuffer.Create(&UploadContext.CommandPool);

    UploadContext.UploadFence.Create(GetDevice());
    UploadContext.UploadFence.Reset();
}

void FxRenderBackend::DestroyUploadContext()
{
    UploadContext.UploadFence.Destroy();
    UploadContext.CommandBuffer.Destroy();
    UploadContext.CommandPool.Destroy();
}

void FxRenderBackend::InitFrames()
{
    Frames.InitSize(RendererFramesInFlight);

    const uint32 graphics_family = GetDevice()->mQueueFamilies.GetGraphicsFamily();

    RvkGpuDevice *device = GetDevice();

    for (int i = 0; i < Frames.Size; i++) {
        RvkFrameData& frame = Frames.Data[i];
        frame.CommandPool.Create(device, graphics_family);
        frame.CommandBuffer.Create(&frame.CommandPool);
        frame.CompCommandBuffer.Create(&frame.CommandPool);
        frame.LightCommandBuffer.Create(&frame.CommandPool);

        frame.Create(device);
    }
}

void FxRenderBackend::DestroyFrames()
{
    vkQueueWaitIdle(GetDevice()->GraphicsQueue);

    for (auto &frame : Frames) {
        // frame.DescriptorSet.Destroy();
        frame.CompDescriptorSet.Destroy();

        frame.CommandBuffer.Destroy();
        frame.CompCommandBuffer.Destroy();
        frame.LightCommandBuffer.Destroy();
        frame.CommandPool.Destroy();

        frame.Destroy();
    }

    Frames.Free();
}

void FxRenderBackend::InitVulkan()
{
    const char* app_name = "Foxtrot";
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = app_name;
    app_info.pEngineName = app_name;
    app_info.apiVersion = VK_MAKE_VERSION(1, 3, 261);

    ExtensionNames requested_extensions = {
        VK_EXT_LAYER_SETTINGS_EXTENSION_NAME,
    };

    ExtensionNames all_extensions = MakeInstanceExtensionList(requested_extensions);

#ifdef FX_VULKAN_DEBUG
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

        FxModulePanic("Missing required instance extensions", 0);
    }

    // auto validation_layers = GetAvailableValidationLayers();
    // for (auto &layer : validation_layers) {
    //     Log::Debug("Layer: %s", layer.layerName);
    // }

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
        FxModulePanic("Could not create vulkan instance!", result);
    }

#ifdef FX_VULKAN_DEBUG
    mDebugMessenger = CreateDebugMessenger(mInstance);
    if (!mDebugMessenger) {
        FxModulePanic("Could not create debug messenger", 0);
    }
#endif

    Initialized = true;
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
        // Log::Info(fmt, message);
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
        Log::Error("Could not create debug messenger! (err: %s)", RvkUtil::ResultToStr(status));
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

void FxRenderBackend::InitGPUAllocator()
{
    const RvkGpuDevice *device = GetDevice();

    const VmaAllocatorCreateInfo create_info = {
        .physicalDevice = device->Physical,
        .device = device->Device,
        .instance = mInstance
    };

    const VkResult status = vmaCreateAllocator(&create_info, &GpuAllocator);
    if (status != VK_SUCCESS) {
        FxModulePanic("Could not create VMA allocator!", status);
    }
}

void FxRenderBackend::DestroyGPUAllocator()
{
    vmaDestroyAllocator(GpuAllocator);
}

ExtensionNames FxRenderBackend::MakeInstanceExtensionList(ExtensionNames &user_requested_extensions)
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

ExtensionList &FxRenderBackend::QueryInstanceExtensions(bool invalidate_previous)
{
    Log::Debug("Query Extensions", 0);


    if (mAvailableExtensions.IsNotEmpty()) {
        if (invalidate_previous) {
            mAvailableExtensions.Free();
        }
        else {
            return mAvailableExtensions;
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
    mAvailableExtensions.InitSize(extension_count);

    // Get the available instance extensions
    result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, mAvailableExtensions.Data);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Could not query instance extensions!");
    }

    // std::cout << "=== Available Instance Extensions (" << extension_count << ") ===\n";

    // for (uint32_t i = 0; i < extension_count; ++i) {
    //     const VkExtensionProperties& extension = mAvailableExtensions[i];
    //     std::cout << extension.extensionName << " : " << extension.specVersion << "\n";
    // }

    return mAvailableExtensions;
}

void FxRenderBackend::SubmitUploadCmd(FxRenderBackend::SubmitFunc upload_func)
{
    RvkCommandBuffer &cmd = UploadContext.CommandBuffer;

    cmd.Record(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    upload_func(cmd);
    cmd.End();

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .commandBufferCount = 1,
        .pCommandBuffers = &cmd.CommandBuffer,
    };

    // Log::Debug(
    //     "cmd thread: %lu",
    //     std::this_thread::get_id()
    // );

    VkTry(
        vkQueueSubmit(GetDevice()->TransferQueue, 1, &submit_info, UploadContext.UploadFence.Fence),
        "Error submitting upload buffer"
    );

    UploadContext.UploadFence.WaitFor();
    UploadContext.UploadFence.Reset();

    UploadContext.CommandPool.Reset();
}


void FxRenderBackend::SubmitOneTimeCmd(FxRenderBackend::SubmitFunc submit_func)
{
    RvkCommandBuffer cmd;
    cmd.Create(&GetFrame()->CommandPool);

    cmd.Record(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    submit_func(cmd);
    cmd.End();

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .commandBufferCount = 1,
        .pCommandBuffers = &cmd.CommandBuffer,
    };

    VkTry(
        vkQueueSubmit(GetDevice()->GraphicsQueue, 1, &submit_info, nullptr),
        "Error submitting upload buffer"
    );

    vkQueueWaitIdle(GetDevice()->GraphicsQueue);

    cmd.Reset();
    cmd.Destroy();
}



FrameResult FxRenderBackend::BeginFrame(FxDeferredRenderer& renderer)
{
    RvkFrameData *frame = GetFrame();

    CurrentGPass = renderer.GetCurrentGPass();
    CurrentCompPass = renderer.GetCurrentCompPass();
    CurrentLightingPass = renderer.GetCurrentLightingPass();

    // memcpy(GetUbo().MvpMatrix.RawData, MVPMatrix.RawData, sizeof(Mat4f));

    frame->InFlight.WaitFor();

    FrameResult result = GetNextSwapchainImage(frame);
    if (result != FrameResult::Success) {
        return result;
    }

    frame->InFlight.Reset();

    frame->CommandBuffer.Reset();
    frame->CommandBuffer.Record();

    // pipeline.RenderPass.Begin();
    // pipeline.Bind(frame->CommandBuffer);

    CurrentGPass->Begin();


    const int32 width = Swapchain.Extent.Width();
    const int32 height = Swapchain.Extent.Height();

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

    FxDrawPushConstants push_constants{};
    // memcpy(push_constants.MVPMatrix, MVPMatrix.RawData, sizeof(float32) * 16);
    vkCmdPushConstants(frame->CommandBuffer.CommandBuffer, renderer.GPassPipeline.Layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants), &push_constants);

    return FrameResult::Success;
}

void FxRenderBackend::PresentFrame()
{
    RvkFrameData *frame = GetFrame();

    const VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame->LightingSem.Semaphore,
        .pWaitDstStageMask = wait_stages,

        // command buffers
        .commandBufferCount = 1,
        .pCommandBuffers = &frame->CompCommandBuffer.CommandBuffer,

        // signal semaphores
        .signalSemaphoreCount = 1,

        // .pSignalSemaphores = &frame->RenderFinished.Semaphore
        .pSignalSemaphores = &frame->RenderFinished.Semaphore
    };

    VkTry(
        vkQueueSubmit(GetDevice()->GraphicsQueue, 1, &submit_info, frame->InFlight.Fence),
        "Error submitting draw buffer"
    );

    if (Swapchain.Initialized != true) {
        FxModulePanic("Swapchain not initialized!", 0);
    }

    const VkSwapchainKHR swapchains[] = {
        Swapchain.GetSwapchain(),
    };

    const VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame->RenderFinished.Semaphore,

        .swapchainCount = 1,
        .pSwapchains = swapchains,

        .pImageIndices = &mImageIndex,

        .pResults = nullptr,
    };

    const VkResult status = vkQueuePresentKHR(GetDevice()->PresentQueue, &present_info);

    if (status == VK_SUCCESS) { }
    else if (status == VK_ERROR_OUT_OF_DATE_KHR || status == VK_SUBOPTIMAL_KHR) {
        // Swapchain.Rebuild()..
    }
    else {
        Log::Error("Error submitting present queue", status);
    }
}

void FxRenderBackend::BeginLighting()
{
    RvkFrameData* frame = GetFrame();

    CurrentGPass->End();
    frame->CommandBuffer.End();

    frame->LightCommandBuffer.Reset();
    frame->LightCommandBuffer.Record();

    CurrentLightingPass->Begin();

    const int32 width = Swapchain.Extent.Width();
    const int32 height = Swapchain.Extent.Height();

    const VkViewport viewport = {
        .x = 0, .y = 0,
        .width = (float32)width,
        .height = (float32)height,
        .minDepth = 0.0,
        .maxDepth = 1.0,
    };

    vkCmdSetViewport(frame->LightCommandBuffer.CommandBuffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = { .x = 0, .y = 0 },
        .extent = { .width = (uint32)width, .height = (uint32)height }
    };

    vkCmdSetScissor(frame->LightCommandBuffer.CommandBuffer, 0, 1, &scissor);

}


void FxRenderBackend::DoComposition()
{
    RvkFrameData* frame = GetFrame();


    CurrentLightingPass->End();
    frame->LightCommandBuffer.End();

    CurrentCompPass->Begin();

    const int32 width = Swapchain.Extent.Width();
    const int32 height = Swapchain.Extent.Height();

    const VkViewport viewport = {
        .x = 0, .y = 0,
        .width = (float32)width,
        .height = (float32)height,
        .minDepth = 0.0,
        .maxDepth = 1.0,
    };

    vkCmdSetViewport(frame->CompCommandBuffer.CommandBuffer, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = { .x = 0, .y = 0 },
        .extent = { .width = (uint32)width, .height = (uint32)height }
    };

    vkCmdSetScissor(frame->CompCommandBuffer.CommandBuffer, 0, 1, &scissor);

    CurrentCompPass->DoCompPass();

    CurrentGPass->Submit();

    CurrentLightingPass->Submit();
    PresentFrame();

    ProcessDeletionQueue();

    mInternalFrameCounter++;

    // better to do one division per frame as opposed to one every GetFrameNumber()
    mFrameNumber = (mInternalFrameCounter) % RendererFramesInFlight;
}

FrameResult FxRenderBackend::GetNextSwapchainImage(RvkFrameData *frame)
{
    const uint64 timeout = UINT64_MAX; // TODO: change this value and handle AcquireNextImage errors correctly

    const VkResult result = vkAcquireNextImageKHR(
        GetDevice()->Device,
        Swapchain.GetSwapchain(),
        timeout,
        frame->ImageAvailable.Semaphore,
        nullptr,
        &mImageIndex
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

inline RvkUniformBufferObject& FxRenderBackend::GetUbo()
{
    static RvkUniformBufferObject ubo;
    return ubo;
}

void FxRenderBackend::CreateSurfaceFromWindow()
{
    if (mWindow == nullptr) {
        FxModulePanic("No window attached! use FxRenderBackend::SelectWindow()", 0);
    }

    bool success = SDL_Vulkan_CreateSurface(mWindow->GetWindow(), mInstance, nullptr, &mWindowSurface);

    if (!success) {
        FxModulePanic("Could not attach Vulkan instance to window! (SDL err: %s)", SDL_GetError());
    }
}


void FxRenderBackend::Destroy()
{
    GetDevice()->WaitForIdle();

    DestroyUploadContext();
    DestroyFrames();

    while (!mDeletionQueue.empty()) {
        ProcessDeletionQueue(true);
        // insert a small delay to avoid the processor spinning out while
        // waiting for an object. this allows handing the core off to other threads.
        std::this_thread::sleep_for(std::chrono::nanoseconds(100));
    }

    // GPassDescriptorPool.Destroy();
    CompDescriptorPool.Destroy();

    GetDevice()->WaitForIdle();

    Swapchain.Destroy();

    DestroyGPUAllocator();

    if (mWindowSurface) {
        vkDestroySurfaceKHR(mInstance, mWindowSurface, nullptr);
    }

    GetDevice()->Destroy();

    DestroyDebugMessenger(mInstance, mDebugMessenger);
    vkDestroyInstance(mInstance, nullptr);

    Initialized = false;
}

RvkFrameData *FxRenderBackend::GetFrame()
{
    return &Frames[GetFrameNumber()];
}
