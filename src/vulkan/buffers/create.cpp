/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "create.h"

#include "image.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/strings.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <vector>

namespace ns::vulkan
{
namespace
{
void check_image_size(
        const VkPhysicalDevice physical_device,
        const VkImageType type,
        const VkExtent3D extent,
        const VkFormat format,
        const VkImageTiling tiling,
        const VkImageUsageFlags usage)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (type)
        {
        case VK_IMAGE_TYPE_1D:
                if (extent.width < 1 || extent.height != 1 || extent.depth != 1)
                {
                        error("Image 1D size error (" + to_string(extent.width) + ", " + to_string(extent.height) + ", "
                              + to_string(extent.depth) + ")");
                }
                break;
        case VK_IMAGE_TYPE_2D:
                if (extent.width < 1 || extent.height < 1 || extent.depth != 1)
                {
                        error("Image 2D size error (" + to_string(extent.width) + ", " + to_string(extent.height) + ", "
                              + to_string(extent.depth) + ")");
                }
                break;
        case VK_IMAGE_TYPE_3D:
                if (extent.width < 1 || extent.height < 1 || extent.depth < 1)
                {
                        error("Image 3D size error (" + to_string(extent.width) + ", " + to_string(extent.height) + ", "
                              + to_string(extent.depth) + ")");
                }
                break;
        default:
                error("Unknown image type " + image_type_to_string(type));
        }
#pragma GCC diagnostic pop

        const VkExtent3D max_extent = find_max_image_extent(physical_device, format, type, tiling, usage);
        if (extent.width > max_extent.width)
        {
                error("Image " + format_to_string(format) + " extent width " + to_string(extent.width)
                      + " is out of range [1, " + to_string(max_extent.width) + "]");
        }
        if (extent.height > max_extent.height)
        {
                error("Image " + format_to_string(format) + " extent height " + to_string(extent.height)
                      + " is out of range [1, " + to_string(max_extent.height) + "]");
        }
        if (extent.depth > max_extent.depth)
        {
                error("Image " + format_to_string(format) + " extent depth " + to_string(extent.depth)
                      + " is out of range [1, " + to_string(max_extent.depth) + "]");
        }
}
}

Buffer create_buffer(
        const VkDevice device,
        const VkDeviceSize size,
        const VkBufferUsageFlags usage,
        std::vector<std::uint32_t> family_indices)
{
        if (size <= 0)
        {
                error("Buffer zero size");
        }

        if (family_indices.empty())
        {
                error("No buffer family indices");
        }

        sort_and_unique(&family_indices);

        VkBufferCreateInfo info = {};

        info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        info.size = size;
        info.usage = usage;

        if (family_indices.size() > 1)
        {
                info.sharingMode = VK_SHARING_MODE_CONCURRENT;
                info.queueFamilyIndexCount = family_indices.size();
                info.pQueueFamilyIndices = family_indices.data();
        }
        else
        {
                info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        return {device, info};
}

Image create_image(
        const VkDevice device,
        const VkPhysicalDevice physical_device,
        const VkImageType type,
        const VkExtent3D extent,
        const VkFormat format,
        std::vector<std::uint32_t> family_indices,
        const VkSampleCountFlagBits samples,
        const VkImageTiling tiling,
        const VkImageUsageFlags usage)
{
        check_image_size(physical_device, type, extent, format, tiling, usage);

        if (family_indices.empty())
        {
                error("No image family indices");
        }

        sort_and_unique(&family_indices);

        VkImageCreateInfo info = {};

        info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        info.imageType = type;
        info.extent = extent;
        info.mipLevels = 1;
        info.arrayLayers = 1;
        info.format = format;
        info.tiling = tiling;
        info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        info.usage = usage;
        info.samples = samples;

        if (family_indices.size() > 1)
        {
                info.sharingMode = VK_SHARING_MODE_CONCURRENT;
                info.queueFamilyIndexCount = family_indices.size();
                info.pQueueFamilyIndices = family_indices.data();
        }
        else
        {
                info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        return {device, info};
}

ImageView create_image_view(const Image& image, const VkImageAspectFlags aspect_flags)
{
        VkImageViewCreateInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        info.image = image.handle();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (image.type())
        {
        case VK_IMAGE_TYPE_1D:
                info.viewType = VK_IMAGE_VIEW_TYPE_1D;
                break;
        case VK_IMAGE_TYPE_2D:
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                break;
        case VK_IMAGE_TYPE_3D:
                info.viewType = VK_IMAGE_VIEW_TYPE_3D;
                break;
        default:
                error("Unknown image type " + image_type_to_string(image.type()));
        }
#pragma GCC diagnostic pop

        info.format = image.format();

        info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        info.subresourceRange.aspectMask = aspect_flags;
        info.subresourceRange.baseMipLevel = 0;
        info.subresourceRange.levelCount = 1;
        info.subresourceRange.baseArrayLayer = 0;
        info.subresourceRange.layerCount = 1;

        return {image, info};
}
}
