/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "buffers.h"

#include "error.h"
#include "print.h"
#include "query.h"
#include "queue.h"
#include "sync.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/image/conversion.h>
#include <src/image/swap.h>

#include <cstring>
#include <sstream>

namespace ns::vulkan
{
namespace
{
Buffer create_buffer(
        const VkDevice& device,
        const VkDeviceSize& size,
        const VkBufferUsageFlags& usage,
        std::vector<uint32_t> family_indices)
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

        return Buffer(device, create_info);
}

VkExtent3D correct_image_extent(const VkImageType& type, const VkExtent3D& extent)
{
        if (type == VK_IMAGE_TYPE_1D)
        {
                VkExtent3D e;
                e.width = extent.width;
                e.depth = 1;
                e.height = 1;
                return e;
        }
        if (type == VK_IMAGE_TYPE_2D)
        {
                VkExtent3D e;
                e.width = extent.width;
                e.height = extent.height;
                e.depth = 1;
                return e;
        }
        if (type == VK_IMAGE_TYPE_3D)
        {
                return extent;
        }
        error("Unknown image type " + image_type_to_string(type));
}

void check_image_size(
        const VkPhysicalDevice& physical_device,
        const VkImageType& type,
        const VkExtent3D& extent,
        const VkFormat& format,
        const VkImageTiling& tiling,
        const VkImageUsageFlags& usage)
{
        if (type == VK_IMAGE_TYPE_1D && (extent.width < 1 || extent.height != 1 || extent.depth != 1))
        {
                error("Image 1D size error (" + to_string(extent.width) + ", " + to_string(extent.height) + ", "
                      + to_string(extent.depth) + ")");
        }
        if (type == VK_IMAGE_TYPE_2D && (extent.width < 1 || extent.height < 1 || extent.depth != 1))
        {
                error("Image 2D size error (" + to_string(extent.width) + ", " + to_string(extent.height) + ", "
                      + to_string(extent.depth) + ")");
        }
        if (type == VK_IMAGE_TYPE_3D && (extent.width < 1 || extent.height < 1 || extent.depth < 1))
        {
                error("Image 3D size error (" + to_string(extent.width) + ", " + to_string(extent.height) + ", "
                      + to_string(extent.depth) + ")");
        }

        const VkExtent3D max_extent = max_image_extent(physical_device, format, type, tiling, usage);
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

Image create_image(
        const VkDevice& device,
        const VkPhysicalDevice& physical_device,
        const VkImageType& type,
        VkExtent3D extent,
        const VkFormat& format,
        std::vector<uint32_t> family_indices,
        const VkSampleCountFlagBits& samples,
        const VkImageTiling& tiling,
        const VkImageUsageFlags& usage)
{
        extent = correct_image_extent(type, extent);

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

        return Image(device, create_info);
}

DeviceMemory create_device_memory(
        const VkDevice& device,
        const VkPhysicalDevice& physical_device,
        const VkBuffer& buffer,
        const VkMemoryPropertyFlags& properties)
{
        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

        VkMemoryAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex =
                physical_device_memory_type_index(physical_device, memory_requirements.memoryTypeBits, properties);

        DeviceMemory device_memory(device, allocate_info);

        VkResult result = vkBindBufferMemory(device, buffer, device_memory, 0);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkBindBufferMemory", result);
        }

        return device_memory;
}

DeviceMemory create_device_memory(
        const VkDevice& device,
        const VkPhysicalDevice& physical_device,
        const VkImage& image,
        const VkMemoryPropertyFlags& properties)
{
        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(device, image, &memory_requirements);

        VkMemoryAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex =
                physical_device_memory_type_index(physical_device, memory_requirements.memoryTypeBits, properties);

        DeviceMemory device_memory(device, allocate_info);

        VkResult result = vkBindImageMemory(device, image, device_memory, 0);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkBindImageMemory", result);
        }

        return device_memory;
}

void copy_host_to_device(
        const DeviceMemory& device_memory,
        const VkDeviceSize& offset,
        const VkDeviceSize& size,
        const void* const data)
{
        void* map_memory_data;

        VkResult result = vkMapMemory(device_memory.device(), device_memory, offset, size, 0, &map_memory_data);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkMapMemory", result);
        }

        std::memcpy(map_memory_data, data, size);

        vkUnmapMemory(device_memory.device(), device_memory);

        // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
}

void copy_device_to_host(
        const DeviceMemory& device_memory,
        const VkDeviceSize& offset,
        const VkDeviceSize& size,
        void* const data)
{
        void* map_memory_data;

        VkResult result = vkMapMemory(device_memory.device(), device_memory, offset, size, 0, &map_memory_data);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkMapMemory", result);
        }

        std::memcpy(data, map_memory_data, size);

        vkUnmapMemory(device_memory.device(), device_memory);

        // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
}

void cmd_copy_buffer_to_image(
        const VkCommandBuffer& command_buffer,
        const VkImage& image,
        const VkBuffer& buffer,
        const VkExtent3D& extent)
{
        VkBufferImageCopy region = {};

        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = extent;

        vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void cmd_copy_image_to_buffer(
        const VkCommandBuffer& command_buffer,
        const VkBuffer& buffer,
        const VkImage& image,
        const VkExtent3D& extent)
{
        VkBufferImageCopy region = {};

        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = extent;

        vkCmdCopyImageToBuffer(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
}

void cmd_transition_texture_layout(
        const VkImageAspectFlags& aspect_mask,
        const VkCommandBuffer& command_buffer,
        const VkImage& image,
        const VkImageLayout& old_layout,
        const VkImageLayout& new_layout)
{
        if (old_layout == new_layout)
        {
                return;
        }

        VkImageMemoryBarrier barrier = {};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;

        barrier.subresourceRange.aspectMask = aspect_mask;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags src_stage;
        VkPipelineStageFlags dst_stage;

        if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else
        {
                barrier.srcAccessMask = 0;
                src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }

        if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
        {
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else
        {
                barrier.dstAccessMask = 0;
                dst_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }

        vkCmdPipelineBarrier(command_buffer, src_stage, dst_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void begin_commands(const VkCommandBuffer& command_buffer)
{
        VkResult result;

        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkBeginCommandBuffer", result);
        }
}

void end_commands(const VkQueue& queue, const VkCommandBuffer& command_buffer)
{
        VkResult result;

        result = vkEndCommandBuffer(command_buffer);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEndCommandBuffer", result);
        }

        queue_submit(command_buffer, queue);
        queue_wait_idle(queue);
}

void staging_buffer_write(
        const VkDevice& device,
        const VkPhysicalDevice& physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const VkBuffer& buffer,
        const VkDeviceSize& offset,
        const VkDeviceSize& size,
        const void* const data)
{
        ASSERT(command_pool.family_index() == queue.family_index());

        Buffer staging_buffer(create_buffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {queue.family_index()}));

        DeviceMemory staging_device_memory(create_device_memory(
                device, physical_device, staging_buffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

        //

        copy_host_to_device(staging_device_memory, 0, size, data);

        CommandBuffer command_buffer(device, command_pool);
        begin_commands(command_buffer);

        VkBufferCopy copy = {};
        copy.srcOffset = 0;
        copy.dstOffset = offset;
        copy.size = size;
        vkCmdCopyBuffer(command_buffer, staging_buffer, buffer, 1, &copy);

        end_commands(queue, command_buffer);
}

void staging_image_write(
        const VkDevice& device,
        const VkPhysicalDevice& physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImage& image,
        const VkImageLayout& old_image_layout,
        const VkImageLayout& new_image_layout,
        const VkExtent3D& extent,
        const std::span<const std::byte>& data)
{
        ASSERT(command_pool.family_index() == queue.family_index());

        const VkDeviceSize size = data_size(data);

        Buffer staging_buffer(create_buffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {queue.family_index()}));

        DeviceMemory staging_device_memory(create_device_memory(
                device, physical_device, staging_buffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

        //

        copy_host_to_device(staging_device_memory, 0, size, data_pointer(data));

        //

        CommandBuffer command_buffer(device, command_pool);
        begin_commands(command_buffer);

        cmd_transition_texture_layout(
                VK_IMAGE_ASPECT_COLOR_BIT, command_buffer, image, old_image_layout,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        cmd_copy_buffer_to_image(command_buffer, image, staging_buffer, extent);

        cmd_transition_texture_layout(
                VK_IMAGE_ASPECT_COLOR_BIT, command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                new_image_layout);

        end_commands(queue, command_buffer);
}

void staging_image_read(
        const VkDevice& device,
        const VkPhysicalDevice& physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImage& image,
        const VkImageLayout& old_image_layout,
        const VkImageLayout& new_image_layout,
        const VkExtent3D& extent,
        const std::span<std::byte>& data)
{
        ASSERT(command_pool.family_index() == queue.family_index());

        const VkDeviceSize size = data_size(data);

        Buffer staging_buffer(create_buffer(device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, {queue.family_index()}));

        DeviceMemory staging_device_memory(create_device_memory(
                device, physical_device, staging_buffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

        //

        CommandBuffer command_buffer(device, command_pool);
        begin_commands(command_buffer);

        cmd_transition_texture_layout(
                VK_IMAGE_ASPECT_COLOR_BIT, command_buffer, image, old_image_layout,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        cmd_copy_image_to_buffer(command_buffer, staging_buffer, image, extent);

        cmd_transition_texture_layout(
                VK_IMAGE_ASPECT_COLOR_BIT, command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                new_image_layout);

        end_commands(queue, command_buffer);

        //

        copy_device_to_host(staging_device_memory, 0, size, data_pointer(data));
}

void transition_texture_layout_color(
        const VkDevice& device,
        const VkCommandPool& command_pool,
        const VkQueue& queue,
        const VkImage& image,
        const VkImageLayout& old_layout,
        const VkImageLayout& new_layout)
{
        if (old_layout == new_layout)
        {
                return;
        }

        CommandBuffer command_buffer(device, command_pool);
        begin_commands(command_buffer);

        cmd_transition_texture_layout(VK_IMAGE_ASPECT_COLOR_BIT, command_buffer, image, old_layout, new_layout);

        end_commands(queue, command_buffer);
}

void transition_texture_layout_depth(
        const VkDevice& device,
        const VkCommandPool& command_pool,
        const VkQueue& queue,
        const VkImage& image,
        const VkImageLayout& old_layout,
        const VkImageLayout& new_layout)
{
        if (old_layout == new_layout)
        {
                return;
        }

        CommandBuffer command_buffer(device, command_pool);
        begin_commands(command_buffer);

        cmd_transition_texture_layout(VK_IMAGE_ASPECT_DEPTH_BIT, command_buffer, image, old_layout, new_layout);

        end_commands(queue, command_buffer);
}

ImageView create_image_view(
        const VkDevice& device,
        const VkImage& image,
        const VkImageType& type,
        const VkFormat& format,
        const VkImageAspectFlags& aspect_flags)
{
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        create_info.image = image;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (type)
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
                error("Unknown image type " + image_type_to_string(type));
        }
#pragma GCC diagnostic pop

        create_info.format = format;

        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        create_info.subresourceRange.aspectMask = aspect_flags;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        return ImageView(device, create_info);
}

void check_pixel_buffer_size(
        const std::span<const std::byte>& pixels,
        const image::ColorFormat& color_format,
        const VkExtent3D& extent)
{
        const std::size_t pixel_size = image::format_pixel_size_in_bytes(color_format);

        if (pixels.size() % pixel_size != 0)
        {
                error("Error pixel buffer size " + to_string(pixels.size()) + " for pixel size "
                      + to_string(pixel_size));
        }

        if (pixels.size() != pixel_size * extent.width * extent.height * extent.depth)
        {
                error("Wrong pixel count " + to_string(pixels.size() / pixel_size) + " for image extent ("
                      + to_string(extent.width) + ", " + to_string(extent.height) + ", " + to_string(extent.depth)
                      + ")");
        }
}

void write_pixels_to_image(
        const VkImage& image,
        const VkFormat& format,
        const VkExtent3D& extent,
        const VkImageLayout& old_image_layout,
        const VkImageLayout& new_image_layout,
        const VkDevice& device,
        const VkPhysicalDevice& physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const image::ColorFormat& color_format,
        const std::span<const std::byte>& pixels)
{
        const auto write = [&](const std::span<const std::byte>& data)
        {
                staging_image_write(
                        device, physical_device, command_pool, queue, image, old_image_layout, new_image_layout, extent,
                        data);
        };

        check_pixel_buffer_size(pixels, color_format, extent);

        image::ColorFormat required_format;
        bool swap = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (format)
        {
        case VK_FORMAT_R8G8B8_SRGB:
                required_format = image::ColorFormat::R8G8B8_SRGB;
                break;
        case VK_FORMAT_B8G8R8_SRGB:
                required_format = image::ColorFormat::R8G8B8_SRGB;
                swap = true;
                break;
        case VK_FORMAT_R8G8B8A8_SRGB:
                required_format = image::ColorFormat::R8G8B8A8_SRGB;
                break;
        case VK_FORMAT_B8G8R8A8_SRGB:
                required_format = image::ColorFormat::R8G8B8A8_SRGB;
                swap = true;
                break;
        case VK_FORMAT_R16G16B16_UNORM:
                required_format = image::ColorFormat::R16G16B16;
                break;
        case VK_FORMAT_R16G16B16A16_UNORM:
                required_format = image::ColorFormat::R16G16B16A16;
                break;
        case VK_FORMAT_R32G32B32_SFLOAT:
                required_format = image::ColorFormat::R32G32B32;
                break;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
                required_format = image::ColorFormat::R32G32B32A32;
                break;
        case VK_FORMAT_R8_SRGB:
                required_format = image::ColorFormat::R8_SRGB;
                break;
        case VK_FORMAT_R16_UNORM:
                required_format = image::ColorFormat::R16;
                break;
        case VK_FORMAT_R32_SFLOAT:
                required_format = image::ColorFormat::R32;
                break;
        default:
                error("Unsupported image format " + format_to_string(format) + " for writing");
        }
#pragma GCC diagnostic pop

        if (color_format == required_format)
        {
                if (!swap)
                {
                        write(pixels);
                        return;
                }
                std::vector<std::byte> buffer{pixels.begin(), pixels.end()};
                image::swap_rb(color_format, buffer);
                check_pixel_buffer_size(buffer, color_format, extent);
                write(buffer);
                return;
        }

        std::vector<std::byte> buffer;
        image::format_conversion(color_format, pixels, required_format, &buffer);
        if (swap)
        {
                image::swap_rb(required_format, buffer);
        }
        check_pixel_buffer_size(buffer, required_format, extent);
        write(buffer);
}

void read_pixels_from_image(
        const VkImage& image,
        const VkFormat& format,
        const VkExtent3D& extent,
        const VkImageLayout& old_image_layout,
        const VkImageLayout& new_image_layout,
        const VkDevice& device,
        const VkPhysicalDevice& physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        image::ColorFormat* const color_format,
        std::vector<std::byte>* const pixels)
{
        bool swap = false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (format)
        {
        case VK_FORMAT_R8G8B8_SRGB:
                *color_format = image::ColorFormat::R8G8B8_SRGB;
                break;
        case VK_FORMAT_B8G8R8_SRGB:
                *color_format = image::ColorFormat::R8G8B8_SRGB;
                swap = true;
                break;
        case VK_FORMAT_R8G8B8A8_SRGB:
                *color_format = image::ColorFormat::R8G8B8A8_SRGB;
                break;
        case VK_FORMAT_B8G8R8A8_SRGB:
                *color_format = image::ColorFormat::R8G8B8A8_SRGB;
                swap = true;
                break;
        case VK_FORMAT_R16G16B16_UNORM:
                *color_format = image::ColorFormat::R16G16B16;
                break;
        case VK_FORMAT_R16G16B16A16_UNORM:
                *color_format = image::ColorFormat::R16G16B16A16;
                break;
        case VK_FORMAT_R32G32B32_SFLOAT:
                *color_format = image::ColorFormat::R32G32B32;
                break;
        case VK_FORMAT_R32G32B32A32_SFLOAT:
                *color_format = image::ColorFormat::R32G32B32A32;
                break;
        case VK_FORMAT_R8_SRGB:
                *color_format = image::ColorFormat::R8_SRGB;
                break;
        case VK_FORMAT_R16_UNORM:
                *color_format = image::ColorFormat::R16;
                break;
        case VK_FORMAT_R32_SFLOAT:
                *color_format = image::ColorFormat::R32;
                break;
        default:
                error("Unsupported image format " + format_to_string(format) + " for reading");
        }
#pragma GCC diagnostic pop

        const std::size_t pixel_size = image::format_pixel_size_in_bytes(*color_format);
        const std::size_t size = pixel_size * extent.width * extent.height * extent.depth;

        pixels->resize(size);

        staging_image_read(
                device, physical_device, command_pool, queue, image, old_image_layout, new_image_layout, extent,
                *pixels);

        if (swap)
        {
                image::swap_rb(*color_format, *pixels);
        }
}

VkFormatFeatureFlags format_features_for_image_usage(VkImageUsageFlags usage, const bool depth)
{
        VkFormatFeatureFlags features = 0;
        if ((usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
        {
                features |= VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
                usage &= ~VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }
        if ((usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        {
                features |= VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
                usage &= ~VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }
        if ((usage & VK_IMAGE_USAGE_SAMPLED_BIT) == VK_IMAGE_USAGE_SAMPLED_BIT)
        {
                features |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
                usage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
        }
        if ((usage & VK_IMAGE_USAGE_STORAGE_BIT) == VK_IMAGE_USAGE_STORAGE_BIT)
        {
                features |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
                usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
        }
        if ((usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        {
                if (depth)
                {
                        error("Usage VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT for depth image");
                }
                features |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
                usage &= ~VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        if ((usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
                features |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
                usage &= ~VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        if (usage != 0)
        {
                std::ostringstream oss;
                oss << "Unsupported image usage " << std::hex << usage;
                error(oss.str());
        }
        return features;
}

VkFormatFeatureFlags format_features_for_color_image_usage(const VkImageUsageFlags& usage)
{
        return format_features_for_image_usage(usage, false /*depth*/);
}

VkFormatFeatureFlags format_features_for_depth_image_usage(const VkImageUsageFlags& usage)
{
        return format_features_for_image_usage(usage, true /*depth*/);
}

template <typename T>
std::string formats_to_sorted_string(const T& formats, const std::string_view& separator)
{
        static_assert(std::is_same_v<VkFormat, typename T::value_type>);
        if (formats.empty())
        {
                return {};
        }
        std::vector<std::string> v;
        v.reserve(formats.size());
        for (VkFormat format : formats)
        {
                v.push_back(format_to_string(format));
        }
        sort_and_unique(&v);
        auto iter = v.cbegin();
        std::string s = *iter;
        while (++iter != v.cend())
        {
                s += separator;
                s += *iter;
        }
        return s;
}

void check_depth_formats(const std::vector<VkFormat>& formats)
{
        const std::unordered_set<VkFormat> depth_formats{
                VK_FORMAT_D16_UNORM, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT,
                VK_FORMAT_D32_SFLOAT_S8_UINT};

        for (VkFormat format : formats)
        {
                if (!depth_formats.contains(format))
                {
                        error("Not depth format " + format_to_string(format) + ", depth formats "
                              + formats_to_sorted_string(depth_formats, ", "));
                }
        }
}
}

BufferWithMemory::BufferWithMemory(
        BufferMemoryType memory_type,
        const Device& device,
        const std::vector<uint32_t>& family_indices,
        VkBufferUsageFlags usage,
        VkDeviceSize size)
        : m_device(device),
          m_physical_device(device.physical_device()),
          m_family_indices(family_indices.cbegin(), family_indices.cend()),
          m_buffer(create_buffer(device, size, usage, family_indices)),
          m_memory_properties(
                  memory_type == BufferMemoryType::HostVisible
                          ? (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                          : (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
          m_device_memory(create_device_memory(device, device.physical_device(), m_buffer, m_memory_properties))
{
        ASSERT(size > 0);
}

void BufferWithMemory::write(
        const CommandPool& command_pool,
        const Queue& queue,
        VkDeviceSize offset,
        VkDeviceSize size,
        const void* data) const
{
        if (offset + size > m_buffer.size())
        {
                error("Offset and data size is greater than buffer size");
        }

        ASSERT(data && !host_visible() && has_usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT));

        if (command_pool.family_index() != queue.family_index())
        {
                error("Buffer command pool family index is not equal to queue family index");
        }
        if (m_family_indices.count(queue.family_index()) == 0)
        {
                error("Queue family index not found in buffer family indices");
        }

        staging_buffer_write(m_device, m_physical_device, command_pool, queue, m_buffer, offset, size, data);
}

void BufferWithMemory::write(const CommandPool& command_pool, const Queue& queue, VkDeviceSize size, const void* data)
        const
{
        if (m_buffer.size() != size)
        {
                error("Buffer size and data size are not equal");
        }

        write(command_pool, queue, 0, size, data);
}

BufferWithMemory::operator VkBuffer() const&
{
        return m_buffer;
}

const vulkan::Buffer& BufferWithMemory::buffer() const
{
        return m_buffer;
}

VkDeviceSize BufferWithMemory::size() const
{
        return m_buffer.size();
}

bool BufferWithMemory::has_usage(VkBufferUsageFlagBits flag) const
{
        return m_buffer.has_usage(flag);
}

VkMemoryPropertyFlags BufferWithMemory::memory_properties() const
{
        return m_memory_properties;
}

bool BufferWithMemory::host_visible() const
{
        return (m_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
}

//

BufferMapper::BufferMapper(const BufferWithMemory& buffer, unsigned long long offset, unsigned long long size)
        : m_device(buffer.m_device_memory.device()), m_device_memory(buffer.m_device_memory), m_size(size)
{
        ASSERT(buffer.host_visible());
        ASSERT(m_size > 0 && offset + m_size <= buffer.size());

        VkResult result = vkMapMemory(m_device, m_device_memory, offset, m_size, 0, &m_pointer);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkMapMemory", result);
        }
}

BufferMapper::BufferMapper(const BufferWithMemory& buffer)
        : m_device(buffer.m_device_memory.device()), m_device_memory(buffer.m_device_memory), m_size(buffer.size())
{
        ASSERT(buffer.host_visible());

        VkResult result = vkMapMemory(m_device, m_device_memory, 0, VK_WHOLE_SIZE, 0, &m_pointer);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkMapMemory", result);
        }
}

BufferMapper::~BufferMapper()
{
        vkUnmapMemory(m_device, m_device_memory);
        // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
}

//

ImageWithMemory::ImageWithMemory(
        const Device& device,
        const std::vector<uint32_t>& family_indices,
        const std::vector<VkFormat>& format_candidates,
        VkSampleCountFlagBits sample_count,
        VkImageType type,
        VkExtent3D extent,
        VkImageUsageFlags usage,
        VkImageLayout layout,
        const CommandPool& command_pool,
        const Queue& queue)
        : m_extent(correct_image_extent(type, extent)),
          m_device(device),
          m_physical_device(device.physical_device()),
          m_family_indices(family_indices.cbegin(), family_indices.cend()),
          m_type(type),
          m_sample_count(sample_count),
          m_usage(usage),
          m_format(find_supported_image_format(
                  m_physical_device,
                  format_candidates,
                  m_type,
                  VK_IMAGE_TILING_OPTIMAL,
                  format_features_for_color_image_usage(m_usage),
                  m_usage,
                  m_sample_count)),
          m_image(create_image(
                  m_device,
                  m_physical_device,
                  m_type,
                  m_extent,
                  m_format,
                  family_indices,
                  m_sample_count,
                  VK_IMAGE_TILING_OPTIMAL,
                  m_usage)),
          m_device_memory(
                  create_device_memory(m_device, m_physical_device, m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
          m_image_view(create_image_view(m_device, m_image, m_type, m_format, VK_IMAGE_ASPECT_COLOR_BIT))
{
        if (layout != VK_IMAGE_LAYOUT_UNDEFINED)
        {
                check_family_index(command_pool, queue);

                transition_texture_layout_color(
                        device, command_pool, queue, m_image, VK_IMAGE_LAYOUT_UNDEFINED, layout);
        }
}

void ImageWithMemory::check_family_index(const CommandPool& command_pool, const Queue& queue) const
{
        if (command_pool.family_index() != queue.family_index())
        {
                error("Command pool family index is not equal to queue family index");
        }
        if (m_family_indices.count(queue.family_index()) == 0)
        {
                error("Queue family index is not found in the image family indices");
        }
}

void ImageWithMemory::write_pixels(
        const CommandPool& command_pool,
        const Queue& queue,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        image::ColorFormat color_format,
        const std::span<const std::byte>& pixels) const
{
        ASSERT((m_usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        check_family_index(command_pool, queue);

        check_pixel_buffer_size(pixels, color_format, m_extent);

        write_pixels_to_image(
                m_image, m_format, m_extent, old_layout, new_layout, m_device, m_physical_device, command_pool, queue,
                color_format, pixels);
}

void ImageWithMemory::read_pixels(
        const CommandPool& command_pool,
        const Queue& queue,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        image::ColorFormat* color_format,
        std::vector<std::byte>* pixels) const
{
        ASSERT((m_usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

        check_family_index(command_pool, queue);

        read_pixels_from_image(
                m_image, m_format, m_extent, old_layout, new_layout, m_device, m_physical_device, command_pool, queue,
                color_format, pixels);
}

VkImage ImageWithMemory::image() const
{
        return m_image;
}

VkImageType ImageWithMemory::type() const
{
        return m_type;
}

VkFormat ImageWithMemory::format() const
{
        return m_format;
}

VkImageView ImageWithMemory::image_view() const
{
        return m_image_view;
}

bool ImageWithMemory::has_usage(VkImageUsageFlags usage) const
{
        return (m_usage & usage) == usage;
}

VkSampleCountFlagBits ImageWithMemory::sample_count() const
{
        return m_sample_count;
}

unsigned ImageWithMemory::width() const
{
        return m_extent.width;
}

unsigned ImageWithMemory::height() const
{
        if (m_type == VK_IMAGE_TYPE_1D)
        {
                error("Image 1D has no height");
        }
        return m_extent.height;
}

unsigned ImageWithMemory::depth() const
{
        if (m_type != VK_IMAGE_TYPE_3D)
        {
                error("Only image 3D has depth");
        }
        return m_extent.depth;
}

VkExtent3D ImageWithMemory::extent() const
{
        return m_extent;
}

//

DepthImageWithMemory::DepthImageWithMemory(
        const Device& device,
        const std::vector<uint32_t>& family_indices,
        const std::vector<VkFormat>& formats,
        VkSampleCountFlagBits sample_count,
        uint32_t width,
        uint32_t height,
        VkImageUsageFlags usage)
{
        if (width <= 0 || height <= 0)
        {
                error("Depth attachment size error");
        }

        check_depth_formats(formats);

        constexpr VkImageTiling TILING = VK_IMAGE_TILING_OPTIMAL;

        m_usage = usage;
        m_format = find_supported_image_format(
                device.physical_device(), formats, VK_IMAGE_TYPE_2D, TILING,
                format_features_for_depth_image_usage(m_usage), m_usage, sample_count);

        const VkExtent3D max_extent =
                max_image_extent(device.physical_device(), m_format, VK_IMAGE_TYPE_2D, TILING, m_usage);
        m_width = std::min(width, max_extent.width);
        m_height = std::min(height, max_extent.height);

        m_image = create_image(
                device, device.physical_device(), VK_IMAGE_TYPE_2D, make_extent(m_width, m_height), m_format,
                family_indices, sample_count, TILING, m_usage);
        m_device_memory =
                create_device_memory(device, device.physical_device(), m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_image_view = create_image_view(device, m_image, VK_IMAGE_TYPE_2D, m_format, VK_IMAGE_ASPECT_DEPTH_BIT);
        m_sample_count = sample_count;
}

DepthImageWithMemory::DepthImageWithMemory(
        const Device& device,
        const std::vector<uint32_t>& family_indices,
        const std::vector<VkFormat>& formats,
        VkSampleCountFlagBits sample_count,
        uint32_t width,
        uint32_t height,
        VkImageUsageFlags usage,
        VkImageLayout layout,
        VkCommandPool command_pool,
        VkQueue queue)
        : DepthImageWithMemory(device, family_indices, formats, sample_count, width, height, usage)
{
        if (layout != VK_IMAGE_LAYOUT_UNDEFINED)
        {
                transition_texture_layout_depth(
                        device, command_pool, queue, m_image, VK_IMAGE_LAYOUT_UNDEFINED, layout);
        }
}

VkImage DepthImageWithMemory::image() const
{
        return m_image;
}

VkFormat DepthImageWithMemory::format() const
{
        return m_format;
}

VkImageView DepthImageWithMemory::image_view() const
{
        return m_image_view;
}

VkImageUsageFlags DepthImageWithMemory::usage() const
{
        return m_usage;
}

VkSampleCountFlagBits DepthImageWithMemory::sample_count() const
{
        return m_sample_count;
}

unsigned DepthImageWithMemory::width() const
{
        return m_width;
}

unsigned DepthImageWithMemory::height() const
{
        return m_height;
}

//

ColorAttachment::ColorAttachment(
        const Device& device,
        const std::vector<uint32_t>& family_indices,
        const std::vector<VkFormat>& format_candidates,
        VkSampleCountFlagBits sample_count,
        uint32_t width,
        uint32_t height)
{
        constexpr VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        constexpr VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        m_format = find_supported_image_format(
                device.physical_device(), format_candidates, VK_IMAGE_TYPE_2D, tiling,
                format_features_for_color_image_usage(usage), usage, sample_count);
        m_image = create_image(
                device, device.physical_device(), VK_IMAGE_TYPE_2D, make_extent(width, height), m_format,
                family_indices, sample_count, tiling, usage);
        m_device_memory =
                create_device_memory(device, device.physical_device(), m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_image_view = create_image_view(device, m_image, VK_IMAGE_TYPE_2D, m_format, VK_IMAGE_ASPECT_COLOR_BIT);
        m_sample_count = sample_count;
}

VkImage ColorAttachment::image() const
{
        return m_image;
}

VkFormat ColorAttachment::format() const
{
        return m_format;
}

VkImageView ColorAttachment::image_view() const
{
        return m_image_view;
}

VkSampleCountFlagBits ColorAttachment::sample_count() const
{
        return m_sample_count;
}
}
