/*
 * File:        BarrierHelper.cpp
 * Author:      emd22
 * Created:     10/08/2026
 * Description: Helper functions for creating barriers for images and buffers
 */

#include "BarrierHelper.hpp"

#include "Image.hpp"

#include <Core/Assert.hpp>
#include <Renderer/Globals.hpp>
#include <Renderer/RenderBackend.hpp>

namespace fx::renderer {
namespace BarrierHelper {

/////////////////////////////////////
// Helpers
/////////////////////////////////////

#define HANDOFF_REQUIRED_CHECK(queue_families_)                                                                        \
	if (!queue_families_.HasIndependentTransfer()) {                                                                   \
		return;                                                                                                        \
	}


static VkImageSubresourceRange SubresourceRange(const Image* image)
{
	return VkImageSubresourceRange {
		.aspectMask = ImageFormatUtil::GetAspectMask(image->Info.Format),
		.baseMipLevel = 0,
		.levelCount = image->Info.MipCount,
		.baseArrayLayer = 0,
		.layerCount = 1,
	};
}


struct LayoutTransitionInfo
{
	VkAccessFlags AccessMask = VK_ACCESS_NONE;
	VkPipelineStageFlags StageMask = VK_PIPELINE_STAGE_NONE;
};


static const LayoutTransitionInfo GetLayoutTransitionInfo(VkImageLayout layout)
{
	switch (layout) {
	case VK_IMAGE_LAYOUT_UNDEFINED:
		return { 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return { VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		return { VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT };

		/////////////////////////////////////
		// Input Attachments
		/////////////////////////////////////

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return { VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT };

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		return { VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
				 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT };


		/////////////////////////////////////
		// Output Targets
		/////////////////////////////////////

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return { VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		return { VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				 VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT };

	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		return { 0, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };

	default:;
		LogError("Unknown image layout!");
		FX_BREAKPOINT;
	}

	return {};
}


/////////////////////////////////////
// Public functions
/////////////////////////////////////

void ImageTransferHandoff(const Image* image)
{
	const renderer::QueueFamilies& q_families = gRenderer->GetDevice()->mQueueFamilies;

	HANDOFF_REQUIRED_CHECK(q_families);

	Assert(image != nullptr);

	const uint32 transfer_queue_index = q_families.GetTransferFamily();
	const uint32 graphics_queue_index = q_families.GetGraphicsFamily();

	VkImageMemoryBarrier2 transfer_barrier {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.pNext = nullptr,
		/*  */
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
		.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
		/*  */
		.dstStageMask = VK_PIPELINE_STAGE_2_NONE,
		.dstAccessMask = VK_ACCESS_2_NONE,
		/*  */
		.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,

		.srcQueueFamilyIndex = transfer_queue_index,
		.dstQueueFamilyIndex = graphics_queue_index,

		.image = image->InternalImage,

		.subresourceRange = SubresourceRange(image),
	};

	VkDependencyInfo dep_info {
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.pNext = nullptr,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &transfer_barrier,
	};

	vkCmdPipelineBarrier2(gRenderer->TransferContext.CmdBuffer, &dep_info);
}

void ImageLayoutTransition(Image* image, VkImageLayout new_layout, CommandBuffer& cmd, uint32 mip_level,
						   uint32 num_levels)
{
	bool is_depth_texture = ImageFormatUtil::IsDepth(image->Info.Format);
	VkImageAspectFlags aspect = ImageFormatUtil::GetAspectMask(image->Info.Format);

	LayoutTransitionInfo src_info = GetLayoutTransitionInfo(image->ImageLayout);
	LayoutTransitionInfo dst_info = GetLayoutTransitionInfo(new_layout);

	VkImageMemoryBarrier barrier {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = nullptr,

		.srcAccessMask = src_info.AccessMask,
		.dstAccessMask = dst_info.AccessMask,

		.oldLayout = image->ImageLayout, .newLayout = new_layout,

		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

		.image = image->InternalImage,

		.subresourceRange = {
			.aspectMask = aspect,
			.baseMipLevel = mip_level,
			.levelCount = num_levels,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	vkCmdPipelineBarrier(cmd, src_info.StageMask, dst_info.StageMask, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	image->ImageLayout = new_layout;
}


} // namespace BarrierHelper
} // namespace fx::renderer
