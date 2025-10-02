
#include "RxImage.hpp"

#include "../RxRenderBackend.hpp"

#include <Core/FxDefines.hpp>
#include <Core/FxPanic.hpp>
#include <Core/FxStackArray.hpp>
#include <FxEngine.hpp>

FX_SET_MODULE_NAME("RxImage")

const RxImageTypeProperties RxImageTypeGetProperties(RxImageType image_type)
{
    RxImageTypeProperties props {};

    if (image_type == RxImageType::Image) {
        props.ViewType = VK_IMAGE_VIEW_TYPE_2D;
        props.LayerCount = 1;
    }
    else if (image_type == RxImageType::Cubemap) {
        props.ViewType = VK_IMAGE_VIEW_TYPE_CUBE;
        props.LayerCount = 6;
    }
    else {
        OldLog::Error("Unknown image type!", 0);
    }

    return props;
}

RxGpuDevice* RxImage::GetDevice()
{
    // If the device has not been created with `::Create` then mDevice will not be
    // set. I think this would be fine, there should probably be a `::Build` or
    // `::Create` method that does this instead though.

    if (mDevice == nullptr) {
        mDevice = gRenderer->GetDevice();
    }

    return mDevice;
}

void RxImage::Create(RxImageType image_type, const FxVec2u& size, VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkImageAspectFlags aspect_flags)
{
    Size = size;
    Format = format;
    mViewType = image_type;
    mDevice = gRenderer->GetDevice();

    if (RxUtil::IsFormatDepth(format)) {
        mIsDepthTexture = true;
    }


    // Get the vulkan values for the image type
    RxImageTypeProperties image_type_props = RxImageTypeGetProperties(image_type);

    VkImageCreateFlags image_create_flags = 0;

    if (image_type == RxImageType::Cubemap) {
        image_create_flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    // Create the vulkan image
    VkImageCreateInfo image_info {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,

        // Dimensions
        .extent.width = size.Width(),
        .extent.height = size.Height(),
        .extent.depth = 1,

        .mipLevels = 1,
        .arrayLayers = image_type_props.LayerCount,
        .format = format,
        .tiling = tiling,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = usage,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,

        .flags = image_create_flags,
    };

    VmaAllocationCreateInfo create_info {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .priority = 1.0f,
    };

    VkResult status = vmaCreateImage(gRenderer->GpuAllocator, &image_info, &create_info, &Image, &Allocation, nullptr);
    if (status != VK_SUCCESS) {
        FxModulePanicVulkan("Could not create vulkan image", status);
    }


    const VkImageViewCreateInfo view_create_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = Image,
        .viewType = image_type_props.ViewType,
        .format = format,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = aspect_flags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = image_type_props.LayerCount,
            },
    };

    status = vkCreateImageView(GetDevice()->Device, &view_create_info, nullptr, &View);
    if (status != VK_SUCCESS) {
        FxModulePanicVulkan("Could not create swapchain image view", status);
    }
}

void RxImage::Create(RxImageType image_type, const FxVec2u& size, VkFormat format, VkImageUsageFlags usage,
                     VkImageAspectFlags aspect_flags)
{
    Create(image_type, size, format, VK_IMAGE_TILING_OPTIMAL, usage, aspect_flags);
}

void RxImage::TransitionLayout(VkImageLayout new_layout, RxCommandBuffer& cmd, uint32 layer_count)
{
    VkImageAspectFlags depth_bits = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

    VkImageMemoryBarrier barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = ImageLayout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = Image,
        .srcAccessMask = (mIsDepthTexture) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .subresourceRange =
            {
                .aspectMask = (mIsDepthTexture) ? depth_bits : VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = layer_count,
            },
    };

    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags dest_stage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    if (ImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (ImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    if (mIsDepthTexture) {
        src_stage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(cmd, src_stage, dest_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    ImageLayout = new_layout;
}


void RxImage::CopyFromBuffer(const RxRawGpuBuffer<uint8>& buffer, VkImageLayout final_layout, FxVec2u size,
                             uint32 base_layer)
{
    Fx_Fwd_SubmitUploadCmd(
        [&](RxCommandBuffer& cmd)
        {
            TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmd);

            VkBufferImageCopy copy {
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = 0,
                    .baseArrayLayer = base_layer,
                    .layerCount = 1,
                },
                .imageExtent =
                    VkExtent3D {
                        .width = size.X,
                        .height = size.Y,
                        .depth = 1,
                    },
            };

            vkCmdCopyBufferToImage(cmd, buffer.Buffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

            TransitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmd);
        });
}

enum CubemapLayer
{
    // Forward,

    Right,
    Left,
    Top,
    Bottom,
    Front,
    Back,
};

inline VkImageAspectFlags GetVulkanAspectFlag(RxImageAspectFlag rx_flag, VkFormat image_format)
{
    if (rx_flag == RxImageAspectFlag::Auto) {
        if (RxUtil::IsFormatDepth(image_format)) {
            rx_flag = RxImageAspectFlag::Depth;
        }

        rx_flag = RxImageAspectFlag::Color;
    }

    return static_cast<VkImageAspectFlags>(rx_flag);
}

void RxImage::CreateLayeredImageFromCubemap(RxImage& cubemap, VkFormat image_format, RxImageCubemapOptions options)
{
    // Here is the type of cubemap we will be reading here:
    //
    // T -> Top, B -> Bottom, L -> Left, R -> Right
    // FW -> Forward, BW -> Backward
    //
    // +-----+-----+-----+-----+
    // |     |  T  |     |     |
    // +-----+-----+-----+-----+
    // |  L  |  FW |  R  |  BW |
    // +-----+-----+-----+-----+
    // |     |  B  |     |     |
    // +-----+-----+-----+-----+
    //
    // Note that it is 4 tiles wide and 3 tiles tall.


    const uint32 tile_width = cubemap.Size.X / 4;
    const uint32 tile_height = cubemap.Size.Y / 3;

    FxAssert(tile_width == tile_height);

    VkImageAspectFlags image_aspect_flag = GetVulkanAspectFlag(options.AspectFlag, image_format);


    Create(RxImageType::Cubemap, FxVec2u(tile_width, tile_height), image_format, VK_IMAGE_TILING_OPTIMAL,
           VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, image_aspect_flag);

    FxStackArray<VkImageCopy, 6> copy_infos;

    VkImageCopy copy_info {
        .dstOffset = { .x = 0, .y = 0 },
        .srcSubresource = { .baseArrayLayer = 0, .layerCount = 1, .aspectMask = image_aspect_flag, },
        .dstSubresource = { .baseArrayLayer = 0, .layerCount = 1, .aspectMask = image_aspect_flag, },

        .extent = { .width = tile_width, .height = tile_height, .depth = 1 },
    };

    // Top
    {
        copy_info.srcOffset = { .x = static_cast<int32>(tile_width), .y = 0 };
        copy_info.dstSubresource.baseArrayLayer = CubemapLayer::Top;

        copy_infos.Insert(copy_info);
    }


    // Left
    {
        copy_info.srcOffset = { .x = 0, .y = static_cast<int32>(tile_height) };
        copy_info.dstSubresource.baseArrayLayer = CubemapLayer::Left;


        copy_infos.Insert(copy_info);
    }

    // Front

    {
        copy_info.srcOffset = { .x = static_cast<int32>(tile_width), .y = static_cast<int32>(tile_height) };
        copy_info.dstSubresource.baseArrayLayer = CubemapLayer::Front;


        copy_infos.Insert(copy_info);
    }

    // Forward
    {
        copy_info.srcOffset = { .x = static_cast<int32>(tile_width) * 2, .y = static_cast<int32>(tile_height) };
        copy_info.dstSubresource.baseArrayLayer = CubemapLayer::Right;

        copy_infos.Insert(copy_info);
    }

    // Back
    {
        copy_info.srcOffset = { .x = static_cast<int32>(tile_width) * 3, .y = static_cast<int32>(tile_height) };
        copy_info.dstSubresource.baseArrayLayer = CubemapLayer::Back;

        copy_infos.Insert(copy_info);
    }

    // Bottom
    {
        copy_info.srcOffset = { .x = static_cast<int32>(tile_width), .y = static_cast<int32>(tile_height) * 2 };
        copy_info.dstSubresource.baseArrayLayer = CubemapLayer::Bottom;

        copy_infos.Insert(copy_info);
    }


    gRenderer->SubmitOneTimeCmd(
        [&](RxCommandBuffer& cmd)
        {
            cubemap.TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, cmd);
            TransitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cmd, 6);

            vkCmdCopyImage(cmd.CommandBuffer, cubemap.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, Image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, copy_infos.Data);

            // VkBufferImageCopy copy {
            //     .bufferOffset = 0,
            //     .bufferRowLength = 0,
            //     .bufferImageHeight = 0,
            //     .imageSubresource {
            //         .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            //         .mipLevel = 0,
            //         .baseArrayLayer = base_layer,
            //         .layerCount = 1,
            //     },
            //     .imageExtent =
            //         VkExtent3D {
            //             .width = size.X,
            //             .height = size.Y,
            //             .depth = 1,
            //         },
            // };

            // vkCmdCopyImage(cmd.CommandBuffer, cubemap.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Image,
            //                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // TransitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmd);
            //
            TransitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cmd, 6);
        });
}

void RxImage::Destroy()
{
    if (View != nullptr) {
        vkDestroyImageView(GetDevice()->Device, View, nullptr);
    }

    if (Image != nullptr && Allocation != nullptr) {
        vmaDestroyImage(gRenderer->GpuAllocator, this->Image, this->Allocation);
    }

    this->Image = nullptr;
    this->Allocation = nullptr;
    this->View = nullptr;

    // Fx_Fwd_AddToDeletionQueue([this](FxDeletionObject* obj) {
    //     if (this->Image == nullptr || this->Allocation == nullptr) {
    //         return;
    //     }

    //     printf("Destroy image!!\n");

    //     vkDestroyImageView(this->mDevice->Device, this->View, nullptr);
    //     vmaDestroyImage(RendererVulkan->GpuAllocator, this->Image,
    //     this->Allocation);

    //     this->Image = nullptr;
    //     this->Allocation = nullptr;
    // });
}
