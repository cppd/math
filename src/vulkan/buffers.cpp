/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "create.h"
#include "error.h"
#include "print.h"
#include "query.h"
#include "queue.h"
#include "sync.h"

#include <src/color/conversion_span.h>
#include <src/com/error.h>
#include <src/com/print.h>

#include <cstring>

namespace vulkan
{
namespace
{
Buffer create_buffer(
        VkDevice device,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        const std::unordered_set<uint32_t>& family_indices)
{
        if (size <= 0)
        {
                error("Buffer zero size");
        }

        if (family_indices.empty())
        {
                error("Buffer family index set is empty");
        }

        const std::vector<uint32_t> indices(family_indices.cbegin(), family_indices.cend());

        VkBufferCreateInfo create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = size;
        create_info.usage = usage;

        if (indices.size() > 1)
        {
                create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
                create_info.queueFamilyIndexCount = indices.size();
                create_info.pQueueFamilyIndices = indices.data();
        }
        else
        {
                create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        return Buffer(device, create_info);
}

VkExtent3D correct_image_extent(VkImageType type, const VkExtent3D& extent)
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
        VkPhysicalDevice physical_device,
        VkImageType type,
        VkExtent3D extent,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage)
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
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkImageType type,
        VkExtent3D extent,
        VkFormat format,
        const std::unordered_set<uint32_t>& family_indices,
        VkSampleCountFlagBits samples,
        VkImageTiling tiling,
        VkImageUsageFlags usage)
{
        extent = correct_image_extent(type, extent);

        check_image_size(physical_device, type, extent, format, tiling, usage);

        if (family_indices.empty())
        {
                error("Image family index set is empty");
        }

        const std::vector<uint32_t> indices(family_indices.cbegin(), family_indices.cend());

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

        if (indices.size() > 1)
        {
                create_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
                create_info.queueFamilyIndexCount = indices.size();
                create_info.pQueueFamilyIndices = indices.data();
        }
        else
        {
                create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        return Image(device, create_info);
}

DeviceMemory create_device_memory(
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkBuffer buffer,
        VkMemoryPropertyFlags properties)
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
        VkDevice device,
        VkPhysicalDevice physical_device,
        VkImage image,
        VkMemoryPropertyFlags properties)
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
        VkDeviceSize offset,
        const void* data,
        VkDeviceSize data_size)
{
        void* map_memory_data;

        VkResult result = vkMapMemory(device_memory.device(), device_memory, offset, data_size, 0, &map_memory_data);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkMapMemory", result);
        }

        std::memcpy(map_memory_data, data, data_size);

        vkUnmapMemory(device_memory.device(), device_memory);

        // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
}

// void copy_device_to_host(const DeviceMemory& device_memory, VkDeviceSize offset, void* data, VkDeviceSize data_size)
//{
//        void* map_memory_data;

//        VkResult result = vkMapMemory(device_memory.device(), device_memory, offset, data_size, 0, &map_memory_data);
//        if (result != VK_SUCCESS)
//        {
//                vulkan_function_error("vkMapMemory", result);
//        }

//        std::memcpy(data, map_memory_data, data_size);

//        vkUnmapMemory(device_memory.device(), device_memory);

//        // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
//}

void cmd_copy_buffer_to_buffer(
        VkCommandBuffer command_buffer,
        VkBuffer dst_buffer,
        VkBuffer src_buffer,
        VkDeviceSize size)
{
        VkBufferCopy copy = {};
        // copy.srcOffset = 0;
        // copy.dstOffset = 0;
        copy.size = size;
        vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy);
}

void cmd_copy_buffer_to_image(VkCommandBuffer command_buffer, VkImage image, VkBuffer buffer, VkExtent3D extent)
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

void cmd_transition_texture_layout(
        VkImageAspectFlags aspect_mask,
        VkCommandBuffer command_buffer,
        VkImage image,
        VkImageLayout old_layout,
        VkImageLayout new_layout)
{
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

        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags destination_stage;

        if (old_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (
                old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                && new_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = 0;

                source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destination_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
        else if (
                old_layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && old_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                && new_layout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
                && new_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && old_layout != new_layout)
        {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = 0;

                source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destination_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
        else
        {
                error("Unsupported texture layout transition, old = " + image_layout_to_string(old_layout)
                      + ", new = " + image_layout_to_string(new_layout));
        }

        vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void begin_commands(VkCommandBuffer command_buffer)
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

void end_commands(VkQueue queue, VkCommandBuffer command_buffer)
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
        VkDevice device,
        VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        VkBuffer buffer,
        VkDeviceSize data_size,
        const void* data)
{
        ASSERT(command_pool.family_index() == queue.family_index());

        Buffer staging_buffer(
                create_buffer(device, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {queue.family_index()}));

        DeviceMemory staging_device_memory(create_device_memory(
                device, physical_device, staging_buffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

        //

        copy_host_to_device(staging_device_memory, 0, data, data_size);

        CommandBuffer command_buffer(device, command_pool);
        begin_commands(command_buffer);

        cmd_copy_buffer_to_buffer(command_buffer, buffer, staging_buffer, data_size);

        end_commands(queue, command_buffer);
}

template <typename T>
void staging_image_write(
        VkDevice device,
        VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        VkImage image,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout,
        VkExtent3D extent,
        const T& data)
{
        ASSERT(command_pool.family_index() == queue.family_index());

        const VkDeviceSize size = data_size(data);
        const void* const pointer = data_pointer(data);

        Buffer staging_buffer(create_buffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {queue.family_index()}));

        DeviceMemory staging_device_memory(create_device_memory(
                device, physical_device, staging_buffer,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

        //

        copy_host_to_device(staging_device_memory, 0, pointer, size);

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

void transition_texture_layout_color(
        VkDevice device,
        VkCommandPool command_pool,
        VkQueue queue,
        VkImage image,
        VkImageLayout old_layout,
        VkImageLayout new_layout)
{
        CommandBuffer command_buffer(device, command_pool);
        begin_commands(command_buffer);

        cmd_transition_texture_layout(VK_IMAGE_ASPECT_COLOR_BIT, command_buffer, image, old_layout, new_layout);

        end_commands(queue, command_buffer);
}

void transition_texture_layout_depth(
        VkDevice device,
        VkCommandPool command_pool,
        VkQueue queue,
        VkImage image,
        VkImageLayout old_layout,
        VkImageLayout new_layout)
{
        CommandBuffer command_buffer(device, command_pool);
        begin_commands(command_buffer);

        cmd_transition_texture_layout(VK_IMAGE_ASPECT_DEPTH_BIT, command_buffer, image, old_layout, new_layout);

        end_commands(queue, command_buffer);
}

ImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
{
        VkImageViewCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = image;

        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
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

template <typename T>
void check_color_buffer_size(const T& pixels, VkExtent3D extent)
{
        if (pixels.size() != 4ull * extent.width * extent.height * extent.depth)
        {
                error("Wrong RGBA pixel component count " + to_string(pixels.size()) + " for image extent ("
                      + to_string(extent.width) + ", " + to_string(extent.height) + ", " + to_string(extent.depth)
                      + ")");
        }
}
template <typename T>
void check_grayscale_buffer_size(const T& pixels, VkExtent3D extent)
{
        if (pixels.size() != 1ull * extent.width * extent.height * extent.depth)
        {
                error("Wrong grayscale pixel component count " + to_string(pixels.size()) + " for image extent ("
                      + to_string(extent.width) + ", " + to_string(extent.height) + ", " + to_string(extent.depth)
                      + ")");
        }
}

void write_srgb_color_pixels_to_image(
        VkImage image,
        VkFormat format,
        VkExtent3D extent,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout,
        VkDevice device,
        VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const Span<const std::uint8_t>& srgb_pixels)
{
        check_color_buffer_size(srgb_pixels, extent);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (format)
        {
        case VK_FORMAT_R16G16B16A16_UNORM:
        {
                const std::vector<uint16_t> buffer =
                        color_conversion::rgba_pixels_from_srgb_uint8_to_rgb_uint16(srgb_pixels);
                staging_image_write(
                        device, physical_device, command_pool, queue, image, old_image_layout, new_image_layout, extent,
                        buffer);
                break;
        }
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        {
                const std::vector<float> buffer =
                        color_conversion::rgba_pixels_from_srgb_uint8_to_rgb_float(srgb_pixels);
                staging_image_write(
                        device, physical_device, command_pool, queue, image, old_image_layout, new_image_layout, extent,
                        buffer);
                break;
        }
        case VK_FORMAT_R8G8B8A8_SRGB:
        {
                staging_image_write(
                        device, physical_device, command_pool, queue, image, old_image_layout, new_image_layout, extent,
                        srgb_pixels);
                break;
        }
        default:
                error("Unsupported image format " + format_to_string(format) + " for sRGB color pixels");
        }
#pragma GCC diagnostic pop
}

void write_srgb_grayscale_pixels_to_image(
        VkImage image,
        VkFormat format,
        VkExtent3D extent,
        VkImageLayout old_image_layout,
        VkImageLayout new_image_layout,
        VkDevice device,
        VkPhysicalDevice physical_device,
        const CommandPool& command_pool,
        const Queue& queue,
        const Span<const std::uint8_t>& srgb_pixels)
{
        check_grayscale_buffer_size(srgb_pixels, extent);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (format)
        {
        case VK_FORMAT_R16_UNORM:
        {
                const std::vector<uint16_t> buffer =
                        color_conversion::grayscale_pixels_from_srgb_uint8_to_rgb_uint16(srgb_pixels);
                staging_image_write(
                        device, physical_device, command_pool, queue, image, old_image_layout, new_image_layout, extent,
                        buffer);
                break;
        }
        case VK_FORMAT_R32_SFLOAT:
        {
                const std::vector<float> buffer =
                        color_conversion::grayscale_pixels_from_srgb_uint8_to_rgb_float(srgb_pixels);
                staging_image_write(
                        device, physical_device, command_pool, queue, image, old_image_layout, new_image_layout, extent,
                        buffer);
                break;
        }
        case VK_FORMAT_R8_SRGB:
        {
                staging_image_write(
                        device, physical_device, command_pool, queue, image, old_image_layout, new_image_layout, extent,
                        srgb_pixels);
                break;
        }
        default:
                error("Unsupported image format " + format_to_string(format) + " for sRGB grayscale pixels");
        }
#pragma GCC diagnostic pop
}
}

BufferWithMemory::BufferWithMemory(
        BufferMemoryType memory_type,
        const Device& device,
        const std::unordered_set<uint32_t>& family_indices,
        VkBufferUsageFlags usage,
        VkDeviceSize size)
        : m_device(device),
          m_physical_device(device.physical_device()),
          m_family_indices(family_indices),
          m_buffer(create_buffer(
                  device,
                  size,
                  (memory_type == BufferMemoryType::HostVisible) ? usage : (usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT),
                  family_indices)),
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
        VkDeviceSize data_size,
        const void* data_pointer) const
{
        if (m_buffer.size() != data_size)
        {
                error("Buffer size and data size are not equal");
        }

        ASSERT(data_pointer && !host_visible() && usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT));

        if (command_pool.family_index() != queue.family_index())
        {
                error("Buffer command pool family index is not equal to queue family index");
        }
        if (m_family_indices.count(queue.family_index()) == 0)
        {
                error("Queue family index not found in buffer family indices");
        }

        staging_buffer_write(m_device, m_physical_device, command_pool, queue, m_buffer, data_size, data_pointer);
}

BufferWithMemory::operator VkBuffer() const&
{
        return m_buffer;
}

VkDeviceSize BufferWithMemory::size() const
{
        return m_buffer.size();
}

const vulkan::Buffer& BufferWithMemory::buffer() const
{
        return m_buffer;
}

bool BufferWithMemory::usage(VkBufferUsageFlagBits flag) const
{
        return m_buffer.usage(flag);
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

BufferMapper::BufferMapper(const BufferWithMemory& buffer, unsigned long long offset, unsigned long long length)
        : m_device(buffer.m_device_memory.device()), m_device_memory(buffer.m_device_memory), m_length(length)
{
        ASSERT(buffer.host_visible());
        ASSERT(length > 0 && offset + length <= buffer.size());

        VkResult result = vkMapMemory(m_device, m_device_memory, offset, m_length, 0, &m_pointer);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkMapMemory", result);
        }
}

BufferMapper::BufferMapper(const BufferWithMemory& buffer)
        : m_device(buffer.m_device_memory.device()), m_device_memory(buffer.m_device_memory), m_length(buffer.size())
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

// VkFormat
// {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT}
// {VK_FORMAT_R8_SRGB, VK_FORMAT_R16_UNORM, VK_FORMAT_R32_SFLOAT}
ImageWithMemory::ImageWithMemory(
        const Device& device,
        const CommandPool& command_pool,
        const Queue& queue,
        const std::unordered_set<uint32_t>& family_indices,
        const std::vector<VkFormat>& format_candidates,
        VkSampleCountFlagBits sample_count,
        VkImageType type,
        VkExtent3D extent,
        VkImageLayout image_layout,
        bool storage)
        : m_extent(correct_image_extent(type, extent)),
          m_device(device),
          m_physical_device(device.physical_device()),
          m_family_indices(family_indices),
          m_type(type),
          m_sample_count(sample_count),
          m_usage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                  | (storage ? VK_IMAGE_USAGE_STORAGE_BIT : 0)),
          m_format(find_supported_image_format(
                  m_physical_device,
                  format_candidates,
                  m_type,
                  VK_IMAGE_TILING_OPTIMAL,
                  VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT
                          | (storage ? VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT : 0),
                  m_usage,
                  m_sample_count)),
          m_image(create_image(
                  m_device,
                  m_physical_device,
                  m_type,
                  m_extent,
                  m_format,
                  m_family_indices,
                  m_sample_count,
                  VK_IMAGE_TILING_OPTIMAL,
                  m_usage)),
          m_device_memory(
                  create_device_memory(m_device, m_physical_device, m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
          m_image_view(create_image_view(m_device, m_image, m_format, VK_IMAGE_ASPECT_COLOR_BIT))
{
        check_family_index(command_pool, queue);

        if (image_layout != VK_IMAGE_LAYOUT_UNDEFINED)
        {
                transition_texture_layout_color(
                        device, command_pool, queue, m_image, VK_IMAGE_LAYOUT_UNDEFINED, image_layout);
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

void ImageWithMemory::write_srgb_color_pixels(
        const CommandPool& command_pool,
        const Queue& queue,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        const Span<const std::uint_least8_t>& srgb_pixels) const
{
        check_family_index(command_pool, queue);

        write_srgb_color_pixels_to_image(
                m_image, m_format, m_extent, old_layout, new_layout, m_device, m_physical_device, command_pool, queue,
                srgb_pixels);
}

void ImageWithMemory::write_srgb_grayscale_pixels(
        const CommandPool& command_pool,
        const Queue& queue,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        const Span<const std::uint_least8_t>& srgb_pixels) const
{
        check_family_index(command_pool, queue);

        write_srgb_grayscale_pixels_to_image(
                m_image, m_format, m_extent, old_layout, new_layout, m_device, m_physical_device, command_pool, queue,
                srgb_pixels);
}

void ImageWithMemory::clear_commands(VkCommandBuffer command_buffer, VkImageLayout image_layout) const
{
        // Для vkCmdClearColorImage нужно VK_IMAGE_USAGE_TRANSFER_DST_BIT

        ASSERT((usage() & VK_IMAGE_USAGE_TRANSFER_DST_BIT) == VK_IMAGE_USAGE_TRANSFER_DST_BIT);

        VkImageMemoryBarrier barrier = {};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        //

        barrier.oldLayout = image_layout;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrier);

        //

        VkClearColorValue clear_color = clear_color_image_value(m_format);
        VkImageSubresourceRange range = barrier.subresourceRange;

        vkCmdClearColorImage(command_buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &range);

        //

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = image_layout;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(
                command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                nullptr, 1, &barrier);
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

VkImageUsageFlags ImageWithMemory::usage() const
{
        return m_usage;
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

DepthAttachment::DepthAttachment(
        const Device& device,
        const std::unordered_set<uint32_t>& family_indices,
        const std::vector<VkFormat>& formats,
        VkSampleCountFlagBits samples,
        uint32_t width,
        uint32_t height,
        bool sampled)
{
        if (width <= 0 || height <= 0)
        {
                error("Depth attachment size error");
        }

        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkFormatFeatureFlags features =
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | (sampled ? VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT : 0);
        m_usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | (sampled ? VK_IMAGE_USAGE_SAMPLED_BIT : 0);

        m_format = find_supported_image_format(
                device.physical_device(), formats, VK_IMAGE_TYPE_2D, tiling, features, m_usage, samples);

        VkExtent3D max_extent = max_image_extent(device.physical_device(), m_format, VK_IMAGE_TYPE_2D, tiling, m_usage);
        m_width = std::min(width, max_extent.width);
        m_height = std::min(height, max_extent.height);
        m_image = create_image(
                device, device.physical_device(), VK_IMAGE_TYPE_2D, make_extent(m_width, m_height), m_format,
                family_indices, samples, tiling, m_usage);
        m_device_memory =
                create_device_memory(device, device.physical_device(), m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_image_view = create_image_view(device, m_image, m_format, VK_IMAGE_ASPECT_DEPTH_BIT);
        m_sample_count = samples;
}

DepthAttachment::DepthAttachment(
        const Device& device,
        const std::unordered_set<uint32_t>& family_indices,
        const std::vector<VkFormat>& formats,
        VkSampleCountFlagBits samples,
        uint32_t width,
        uint32_t height,
        bool sampled,
        VkCommandPool graphics_command_pool,
        VkQueue graphics_queue,
        VkImageLayout image_layout)
        : DepthAttachment(device, family_indices, formats, samples, width, height, sampled)
{
        if (image_layout != VK_IMAGE_LAYOUT_UNDEFINED)
        {
                transition_texture_layout_depth(
                        device, graphics_command_pool, graphics_queue, m_image, VK_IMAGE_LAYOUT_UNDEFINED,
                        image_layout);
        }
}

VkImage DepthAttachment::image() const
{
        return m_image;
}

VkFormat DepthAttachment::format() const
{
        return m_format;
}

VkImageView DepthAttachment::image_view() const
{
        return m_image_view;
}

VkImageUsageFlags DepthAttachment::usage() const
{
        return m_usage;
}

VkSampleCountFlagBits DepthAttachment::sample_count() const
{
        return m_sample_count;
}

unsigned DepthAttachment::width() const
{
        return m_width;
}

unsigned DepthAttachment::height() const
{
        return m_height;
}

//

ColorAttachment::ColorAttachment(
        const Device& device,
        const std::unordered_set<uint32_t>& family_indices,
        VkFormat format,
        VkSampleCountFlagBits samples,
        uint32_t width,
        uint32_t height)
{
        std::vector<VkFormat> candidates = {format}; // должен быть только этот формат
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        m_format = find_supported_image_format(
                device.physical_device(), candidates, VK_IMAGE_TYPE_2D, tiling, features, usage, samples);
        m_image = create_image(
                device, device.physical_device(), VK_IMAGE_TYPE_2D, make_extent(width, height), m_format,
                family_indices, samples, tiling, usage);
        m_device_memory =
                create_device_memory(device, device.physical_device(), m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_image_view = create_image_view(device, m_image, m_format, VK_IMAGE_ASPECT_COLOR_BIT);
        m_sample_count = samples;

        ASSERT(m_format == format);
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
