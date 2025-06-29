#include <vulkan/vulkan.h>
#include <assert.h>

#include <Renderer/Renderer.hpp>

#include "RvkDynamicRendering.hpp"
#include "RvkSwapchain.hpp"

#include <Core/FxPanic.hpp>

FX_SET_MODULE_NAME("RvkDynamicRendering")


void RvkDynamicRendering::Begin() {
    RvkFrameData *frame = Renderer->GetFrame();

    const RvkSwapchain& swapchain = Renderer->Swapchain;
    const uint32 frame_index = Renderer->GetFrameNumber();

    const VkRenderingAttachmentInfo color_attachment_info {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = swapchain.ImageViews[frame_index],
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = {
            .color = { { 0.0f, 0.0f, 0.0f, 1.0f } }
        }
    };

    // Add other attachments here

    const VkRenderingAttachmentInfo color_attachments[] = {
        color_attachment_info,
        // List other attachments here
    };


    const VkRenderingAttachmentInfo depth_attachment_info {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = Renderer->Swapchain.DepthImages[frame_index].View,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue {
            .depthStencil = {
                .depth = 1.0f,
                .stencil = 0,
            }
        }
    };

    const VkRenderingInfo render_info {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .colorAttachmentCount = sizeof(color_attachments) / sizeof(color_attachments[0]),
        .pColorAttachments = color_attachments,
        .pDepthAttachment = &depth_attachment_info,
        .renderArea = {
            .offset = { 0, 0 },
            .extent = swapchain.Extent,
        },
        .layerCount = 1,
    };

    CommandBuffer = &frame->CommandBuffer;

    assert(CommandBuffer->CommandBuffer != nullptr);
    vkCmdBeginRendering(CommandBuffer->CommandBuffer, &render_info);
}

void RvkDynamicRendering::End() {
    assert(CommandBuffer != nullptr);

    vkCmdEndRendering(CommandBuffer->CommandBuffer);
}
