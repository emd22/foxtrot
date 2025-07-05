#include <vulkan/vulkan.h>
#include <stdexcept>

#include <Renderer/Renderer.hpp>

#include "RvkDevice.hpp"
#include "RvkRenderPass.hpp"

#include "RvkSwapchain.hpp"
#include "vulkan/vulkan_core.h"

#include <Core/FxPanic.hpp>

FX_SET_MODULE_NAME("RvkRenderPass")

void RvkRenderPass::Create(
    const RvkGpuDevice& device,
    const RvkSwapchain& swapchain,
    FxSizedArray<VkAttachmentDescription>& color_attachments,
    VkAttachmentDescription depth_attachment
)
{
    // VkAttachmentDescription color_attachment {
    //     .format = swapchain.SurfaceFormat.format,
    //     .samples = VK_SAMPLE_COUNT_1_BIT,
    //     .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    //     .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    //     .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    //     .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    //     .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    //     .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    // };

    // VkAttachmentDescription depth_attachment {
    //     .format = VK_FORMAT_D16_UNORM,
    //     .samples = VK_SAMPLE_COUNT_1_BIT,
    //     .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    //     .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    //     .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    //     .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    //     .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    //     .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    // };

    FxSizedArray<VkAttachmentReference> color_attachment_refs(color_attachments.Size);

    // Create color attachments ref
    uint32 i = 0;
    for (i = 0; i < color_attachments.Size; i++) {
        // VkAttachmentDescription& attachment = color_attachments[i];

        VkAttachmentReference* ref = color_attachment_refs.Insert();
        ref->attachment = i;
        ref->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    // Create depth attachment ref
    VkAttachmentReference depth_attachment_ref {
        .attachment = i,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    // VkAttachmentReference color_attachment_ref {
    //     .attachment = 0,
    //     .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    // };

    // VkAttachmentReference depth_attachment_ref {
    //     .attachment = 1,
    //     .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    // };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32>(color_attachments.Size),
        .pColorAttachments = color_attachment_refs.Data,
        .pDepthStencilAttachment = &depth_attachment_ref,
        .pResolveAttachments = nullptr,
    };

    VkSubpassDependency subpassDependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
            | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    FxSizedArray<VkAttachmentDescription> all_attachments(color_attachments.Size + 1);
    for (VkAttachmentDescription& attachment : color_attachments) {
        all_attachments.Insert(attachment);
    }
    all_attachments.Insert(depth_attachment);


    VkRenderPassCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32>(all_attachments.Size),
        .pAttachments = all_attachments.Data,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &subpassDependency,
    };

    const VkResult status = vkCreateRenderPass(device.Device, &create_info, nullptr, &RenderPass);
    if (status != VK_SUCCESS) {
        FxModulePanic("Failed to create render pass", status);
    }
}

void RvkRenderPass::Begin() {
    if (RenderPass == nullptr) {
        FxModulePanic("Render pass has not been previously created", 0);
    }

    RvkFrameData *frame = Renderer->GetFrame();
    const auto extent = Renderer->Swapchain.Extent;

    //Log::Debug("Amount of framebuffers: %d", renderer->Swapchain.Framebuffers.Size);

    const VkClearValue clear_values[] = {
        VkClearValue {
            .color = {{1.0f, 1.0f, 1.0f, 1.0f}}
        },
        VkClearValue {
            .depthStencil = {1.0f, 0}
        }
    };
    VkRenderPassBeginInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = RenderPass,
        .framebuffer = Renderer->Swapchain.Framebuffers[Renderer->GetImageIndex()].Framebuffer,
        .renderArea.offset = {0, 0},
        .renderArea.extent = {(uint32)extent.Width(), (uint32)extent.Height()},
        .clearValueCount = sizeof(clear_values) / sizeof(VkClearValue),
        .pClearValues = clear_values,
    };


    CommandBuffer = &frame->CommandBuffer;
    vkCmdBeginRenderPass(CommandBuffer->CommandBuffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
}

void RvkRenderPass::End() {
    if (CommandBuffer == VK_NULL_HANDLE) {
        throw std::runtime_error("Command buffer is null when ending render pass!");
    }

    vkCmdEndRenderPass(CommandBuffer->CommandBuffer);
}

void RvkRenderPass::Destroy(RvkGpuDevice &device) {
    if (RenderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device.Device, RenderPass, nullptr);
    }
}
