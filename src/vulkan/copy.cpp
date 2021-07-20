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

#include "copy.h"

#include "create.h"
#include "error.h"
#include "memory.h"
#include "print.h"
#include "queue.h"
#include "sync.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/image/conversion.h>
#include <src/image/swap.h>

#include <cstring>

namespace ns::vulkan
{
namespace
{
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

void cmd_transition_texture_layout(
        const VkImageAspectFlags& aspect_flags,
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

        barrier.subresourceRange.aspectMask = aspect_flags;
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

void cmd_copy_buffer_to_image(
        const VkImageAspectFlags& aspect_flag,
        const VkCommandBuffer& command_buffer,
        const VkImage& image,
        const VkBuffer& buffer,
        const VkExtent3D& extent)
{
        VkBufferImageCopy region = {};

        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = aspect_flag;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = extent;

        vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void cmd_copy_image_to_buffer(
        const VkImageAspectFlags& aspect_flag,
        const VkCommandBuffer& command_buffer,
        const VkBuffer& buffer,
        const VkImage& image,
        const VkExtent3D& extent)
{
        VkBufferImageCopy region = {};

        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = aspect_flag;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = extent;

        vkCmdCopyImageToBuffer(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
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

void staging_image_write(
        const VkDevice& device,
        const VkPhysicalDevice& physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImage& image,
        const VkImageLayout& old_image_layout,
        const VkImageLayout& new_image_layout,
        const VkImageAspectFlags& aspect_flag,
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
                aspect_flag, command_buffer, image, old_image_layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        cmd_copy_buffer_to_image(aspect_flag, command_buffer, image, staging_buffer, extent);

        cmd_transition_texture_layout(
                aspect_flag, command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, new_image_layout);

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
        const VkImageAspectFlags& aspect_flag,
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
                aspect_flag, command_buffer, image, old_image_layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        cmd_copy_image_to_buffer(aspect_flag, command_buffer, staging_buffer, image, extent);

        cmd_transition_texture_layout(
                aspect_flag, command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, new_image_layout);

        end_commands(queue, command_buffer);

        //

        copy_device_to_host(staging_device_memory, 0, size, data_pointer(data));
}
}

void write_data_to_buffer(
        const VkDevice device,
        const VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const VkBuffer buffer,
        const VkDeviceSize offset,
        const VkDeviceSize size,
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

void write_pixels_to_image(
        const VkDevice device,
        const VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImage image,
        const VkFormat format,
        const VkExtent3D extent,
        const VkImageAspectFlags aspect_flag,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,
        const image::ColorFormat& color_format,
        const std::span<const std::byte>& pixels)
{
        const auto write = [&](const std::span<const std::byte>& data)
        {
                staging_image_write(
                        device, physical_device, command_pool, queue, image, old_layout, new_layout, aspect_flag,
                        extent, data);
        };

        check_pixel_buffer_size(pixels, color_format, extent);

        image::ColorFormat required_format;
        bool swap = false;
        bool color = true;

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
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D16_UNORM_S8_UINT:
                if (aspect_flag != VK_IMAGE_ASPECT_DEPTH_BIT)
                {
                        error("Unsupported image aspect " + to_string(aspect_flag) + " for writing");
                }
                required_format = image::ColorFormat::R16;
                color = false;
                break;
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
                if (aspect_flag != VK_IMAGE_ASPECT_DEPTH_BIT)
                {
                        error("Unsupported image aspect " + to_string(aspect_flag) + " for writing");
                }
                required_format = image::ColorFormat::R32;
                color = false;
                break;
        default:
                error("Unsupported image format " + format_to_string(format) + " for writing");
        }
#pragma GCC diagnostic pop

        ASSERT(!color || aspect_flag == VK_IMAGE_ASPECT_COLOR_BIT);

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
        const VkDevice device,
        const VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const VkImage image,
        const VkFormat format,
        const VkExtent3D extent,
        const VkImageAspectFlags aspect_flag,
        const VkImageLayout old_layout,
        const VkImageLayout new_layout,
        image::ColorFormat* const color_format,
        std::vector<std::byte>* const pixels)
{
        bool swap = false;
        bool color = true;

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
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D16_UNORM_S8_UINT:
                if (aspect_flag != VK_IMAGE_ASPECT_DEPTH_BIT)
                {
                        error("Unsupported image aspect " + to_string(aspect_flag) + " for reading");
                }
                *color_format = image::ColorFormat::R16;
                color = false;
                break;
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
                if (aspect_flag != VK_IMAGE_ASPECT_DEPTH_BIT)
                {
                        error("Unsupported image aspect " + to_string(aspect_flag) + " for reading");
                }
                *color_format = image::ColorFormat::R32;
                color = false;
                break;
        default:
                error("Unsupported image format " + format_to_string(format) + " for reading");
        }
#pragma GCC diagnostic pop

        ASSERT(!color || aspect_flag == VK_IMAGE_ASPECT_COLOR_BIT);

        const std::size_t pixel_size = image::format_pixel_size_in_bytes(*color_format);
        const std::size_t size = pixel_size * extent.width * extent.height * extent.depth;

        pixels->resize(size);

        staging_image_read(
                device, physical_device, command_pool, queue, image, old_layout, new_layout, aspect_flag, extent,
                *pixels);

        if (swap)
        {
                image::swap_rb(*color_format, *pixels);
        }
}
}
