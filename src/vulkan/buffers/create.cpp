/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "../print.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/print.h>

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

        VkBufferCreateInfo create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = size;
        create_info.usage = usage;

        if (family_indices.size() > 1)
        {
                create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
                create_info.queueFamilyIndexCount = family_indices.size();
                create_info.pQueueFamilyIndices = family_indices.data();
        }
        else
        {
                create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        return {device, create_info};
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

        VkImageCreateInfo create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        create_info.imageType = type;
        create_info.extent = extent;
        create_info.mipLevels = 1;
        create_info.arrayLayers = 1;
        create_info.format = format;
        create_info.tiling = tiling;
        create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        create_info.usage = usage;
        create_info.samples = samples;
        // create_info.flags = 0;

        if (family_indices.size() > 1)
        {
                create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
                create_info.queueFamilyIndexCount = family_indices.size();
                create_info.pQueueFamilyIndices = family_indices.data();
        }
        else
        {
                create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        return {device, create_info};
}

ImageView create_image_view(const Image& image, const VkImageAspectFlags aspect_flags)
{
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        create_info.image = image.handle();

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (image.type())
        {
        case VK_IMAGE_TYPE_1D:
                create_info.viewType = VK_IMAGE_VIEW_TYPE_1D;
                break;
        case VK_IMAGE_TYPE_2D:
                create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                break;
        case VK_IMAGE_TYPE_3D:
                create_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
                break;
        default:
                error("Unknown image type " + image_type_to_string(image.type()));
        }
#pragma GCC diagnostic pop

        create_info.format = image.format();

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = aspect_flags;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        return {image, create_info};
}
}
