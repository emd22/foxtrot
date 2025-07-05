#include "RvkRenderTarget.hpp"

#include "../Renderer.hpp"

#define RT_ATTACHMENT_DESC(format_, final_layout_) \
    VkAttachmentDescription { \
        .format = format_, \
        .samples = VK_SAMPLE_COUNT_1_BIT, \
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, \
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE, \
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, \
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, \
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, \
        .finalLayout = final_layout_ \
    }

#define RT_CREATE_COLOR_IMG(img_, img_format_) \
    img_.Create( \
        size, \
        img_format_, \
        VK_IMAGE_TILING_OPTIMAL, \
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, \
        VK_IMAGE_ASPECT_COLOR_BIT \
    );

#define RT_CREATE_DEPTH_IMG(img_, img_format_) \
    img_.Create( \
        size, \
        img_format_, \
        VK_IMAGE_TILING_OPTIMAL, \
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, \
        VK_IMAGE_ASPECT_DEPTH_BIT \
    );

void RvkDeferredRenderTarget::Create(const Vec2u& size, RvkGraphicsPipeline* pipeline)
{
    Size = size;
    mPipeline = pipeline;

    RT_CREATE_COLOR_IMG(Positions, VK_FORMAT_R16G16B16A16_SFLOAT);
    RT_CREATE_COLOR_IMG(Normals, VK_FORMAT_R16G16B16A16_SFLOAT);
    RT_CREATE_COLOR_IMG(Albedo, VK_FORMAT_R8G8B8A8_UNORM);

    RT_CREATE_DEPTH_IMG(Depth, VK_FORMAT_D16_UNORM);

    FxSizedArray<VkAttachmentDescription> color_attachments = {
        RT_ATTACHMENT_DESC(Positions.Format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
        RT_ATTACHMENT_DESC(Normals.Format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
        RT_ATTACHMENT_DESC(Albedo.Format, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
    };

    VkAttachmentDescription depth_attachment = RT_ATTACHMENT_DESC(Depth.Format, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    RenderPass.Create(*Renderer->GetDevice(), Renderer->Swapchain, color_attachments, depth_attachment);

    FxSizedArray<VkImageView> attachment_image_views = {
        Positions.View,
        Normals.View,
        Albedo.View,
        Depth.View
    };

    Framebuffer.Create(attachment_image_views, RenderPass, *pipeline, Size);
}


#define PUSH_IMAGE_IF_SET(img, binding) \
    { \
        VkDescriptorImageInfo image_info { \
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, \
            .imageView = img.View, \
            .sampler = mPipeline->ColorSampler.Sampler, \
        }; \
        VkWriteDescriptorSet image_write { \
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, \
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, \
            .descriptorCount = 1, \
            .dstSet = descriptor_set.Set, \
            .dstBinding = binding, \
            .dstArrayElement = 0, \
            .pImageInfo = &image_info, \
        }; \
        write_infos.Insert(image_write); \
    }


void RvkDeferredRenderTarget::SetupDescriptors()
{
    // assert(!mDescriptorSet.IsInited());
    // mDescriptorSet.Create(Renderer->DescriptorPool, mPipeline->DeferredDescriptorSetLayout);

    // FxSizedArray<VkWriteDescriptorSet> write_infos;
    // write_infos.InitCapacity(4);

    // RvkDescriptorSet& descriptor_set = mDescriptorSet;
    // PUSH_IMAGE_IF_SET(Albedo, 0);
    // PUSH_IMAGE_IF_SET(Normals, 0);

}


void RvkDeferredRenderTarget::Destroy()
{
    Framebuffer.Destroy();
    RenderPass.Destroy(*Renderer->GetDevice());

    Depth.Destroy();
    Albedo.Destroy();
    Normals.Destroy();
    Positions.Destroy();
}


// void RvkRenderTarget::Create(const Vec2u& size, const std::vector<RvkImage>& attachments)
// {
//     Size = size;
//     Attachments = attachments;
// }
