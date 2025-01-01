/*
Copyright (C) 2017-2025 Topological Manifold

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "image.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/vulkan/commands.h>
#include <src/vulkan/error.h>
#include <src/vulkan/strings.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>

namespace ns::vulkan::buffers
{
namespace
{
bool has_bits(const VkImageUsageFlags usage, const VkImageUsageFlagBits bits)
{
        return (usage & bits) == bits;
}
}

VkExtent3D find_max_image_extent(
        const VkPhysicalDevice physical_device,
        const VkFormat format,
        const VkImageType image_type,
        const VkImageTiling tiling,
        const VkImageUsageFlags usage)
{
        VkImageFormatProperties image_properties;
        VULKAN_CHECK(vkGetPhysicalDeviceImageFormatProperties(
                physical_device, format, image_type, tiling, usage, 0 /*VkImageCreateFlags*/, &image_properties));
        return image_properties.maxExtent;
}

VkExtent3D correct_image_extent(const VkImageType type, const VkExtent3D extent)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (type)
        {
        case VK_IMAGE_TYPE_1D:
                return {.width = extent.width, .height = 1, .depth = 1};
        case VK_IMAGE_TYPE_2D:
                return {.width = extent.width, .height = extent.height, .depth = 1};
        case VK_IMAGE_TYPE_3D:
                return extent;
        default:
                error("Unknown image type " + strings::image_type_to_string(type));
        }
#pragma GCC diagnostic pop
}

VkExtent3D limit_image_extent(
        const VkImageType type,
        const VkExtent3D extent,
        const VkPhysicalDevice physical_device,
        const VkFormat format,
        const VkImageTiling tiling,
        const VkImageUsageFlags usage)
{
        const VkExtent3D max_extent = find_max_image_extent(physical_device, format, type, tiling, usage);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (type)
        {
        case VK_IMAGE_TYPE_1D:
                return {.width = std::min(extent.width, max_extent.width), .height = 1, .depth = 1};
        case VK_IMAGE_TYPE_2D:
                return {.width = std::min(extent.width, max_extent.width),
                        .height = std::min(extent.height, max_extent.height),
                        .depth = 1};
        case VK_IMAGE_TYPE_3D:
                return {.width = std::min(extent.width, max_extent.width),
                        .height = std::min(extent.height, max_extent.height),
                        .depth = std::min(extent.depth, max_extent.depth)};
        default:
                error("Unknown image type " + strings::image_type_to_string(type));
        }
#pragma GCC diagnostic pop
}

void transition_image_layout(
        const VkImageAspectFlags aspect_flags,
        const VkDevice device,
        const VkCommandPool command_pool,
        const VkQueue queue,
        const VkImage image,
        const VkImageLayout layout)
{
        ASSERT(layout != VK_IMAGE_LAYOUT_UNDEFINED);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = layout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;

        barrier.subresourceRange.aspectMask = aspect_flags;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;
        const VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        const VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        const auto commands = [&](const VkCommandBuffer command_buffer)
        {
                vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        };

        run_commands(device, command_pool, queue, commands);
}

VkFormatFeatureFlags format_features_for_image_usage(VkImageUsageFlags usage)
{
        VkFormatFeatureFlags features = 0;

        if (has_bits(usage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT))
        {
                features |= VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
                usage &= ~VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if (has_bits(usage, VK_IMAGE_USAGE_TRANSFER_DST_BIT))
        {
                features |= VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
                usage &= ~VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        if (has_bits(usage, VK_IMAGE_USAGE_SAMPLED_BIT))
        {
                features |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
                usage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        if (has_bits(usage, VK_IMAGE_USAGE_STORAGE_BIT))
        {
                features |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
                usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
        }

        if (has_bits(usage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
        {
                features |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
                usage &= ~VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }

        if (has_bits(usage, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT))
        {
                features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
                usage &= ~VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }

        if (usage != 0)
        {
                error("Unsupported image usage " + to_string_binary(usage));
        }

        return features;
}

bool has_usage_for_image_view(const VkImageUsageFlags usage)
{
        return has_bits(usage, VK_IMAGE_USAGE_SAMPLED_BIT) || has_bits(usage, VK_IMAGE_USAGE_STORAGE_BIT)
               || has_bits(usage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
               || has_bits(usage, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
               || has_bits(usage, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
               || has_bits(usage, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
               || has_bits(usage, VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)
               || has_bits(usage, VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT);
}

bool has_usage_for_transfer(const VkImageUsageFlags usage)
{
        return has_bits(usage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT) || has_bits(usage, VK_IMAGE_USAGE_TRANSFER_DST_BIT);
}
}
