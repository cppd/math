/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/color/conversion_span.h"
#include "com/error.h"
#include "com/print.h"

#include <cstring>

namespace
{
vulkan::Buffer create_buffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage,
                             const std::unordered_set<uint32_t>& family_indices)
{
        if (!(size > 0))
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

        return vulkan::Buffer(device, create_info);
}

vulkan::Image create_2d_image(VkDevice device, uint32_t width, uint32_t height, VkFormat format,
                              const std::unordered_set<uint32_t>& family_indices, VkSampleCountFlagBits samples,
                              VkImageTiling tiling, VkImageUsageFlags usage)
{
        if (!(width > 0))
        {
                error("Image zero width");
        }
        if (!(height > 0))
        {
                error("Image zero height");
        }
        if (family_indices.empty())
        {
                error("2D image family index set is empty");
        }

        const std::vector<uint32_t> indices(family_indices.cbegin(), family_indices.cend());

        VkImageCreateInfo create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        create_info.imageType = VK_IMAGE_TYPE_2D;
        create_info.extent.width = width;
        create_info.extent.height = height;
        create_info.extent.depth = 1;
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

        return vulkan::Image(device, create_info);
}

vulkan::DeviceMemory create_device_memory(const vulkan::Device& device, VkBuffer buffer, VkMemoryPropertyFlags properties)
{
        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

        VkMemoryAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex = vulkan::physical_device_memory_type_index(device.physical_device(),
                                                                                  memory_requirements.memoryTypeBits, properties);

        vulkan::DeviceMemory device_memory(device, allocate_info);

        VkResult result = vkBindBufferMemory(device, buffer, device_memory, 0);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkBindBufferMemory", result);
        }

        return device_memory;
}

vulkan::DeviceMemory create_device_memory(const vulkan::Device& device, VkImage image, VkMemoryPropertyFlags properties)
{
        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(device, image, &memory_requirements);

        VkMemoryAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex = vulkan::physical_device_memory_type_index(device.physical_device(),
                                                                                  memory_requirements.memoryTypeBits, properties);

        vulkan::DeviceMemory device_memory(device, allocate_info);

        VkResult result = vkBindImageMemory(device, image, device_memory, 0);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkBindImageMemory", result);
        }

        return device_memory;
}

void copy_host_to_device(const vulkan::DeviceMemory& device_memory, VkDeviceSize offset, const void* data, VkDeviceSize data_size)
{
        void* map_memory_data;

        VkResult result = vkMapMemory(device_memory.device(), device_memory, offset, data_size, 0, &map_memory_data);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkMapMemory", result);
        }

        std::memcpy(map_memory_data, data, data_size);

        vkUnmapMemory(device_memory.device(), device_memory);

        // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
}

// void copy_device_to_host(const vulkan::DeviceMemory& device_memory, VkDeviceSize offset, void* data, VkDeviceSize data_size)
//{
//        void* map_memory_data;

//        VkResult result = vkMapMemory(device_memory.device(), device_memory, offset, data_size, 0, &map_memory_data);
//        if (result != VK_SUCCESS)
//        {
//                vulkan::vulkan_function_error("vkMapMemory", result);
//        }

//        std::memcpy(data, map_memory_data, data_size);

//        vkUnmapMemory(device_memory.device(), device_memory);

//        // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
//}

void begin_commands(VkCommandBuffer command_buffer)
{
        VkResult result;

        VkCommandBufferBeginInfo command_buffer_info = {};
        command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
        }
}

void end_commands(VkQueue queue, VkCommandBuffer command_buffer)
{
        VkResult result;

        result = vkEndCommandBuffer(command_buffer);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkEndCommandBuffer", result);
        }

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = &command_buffer;

        result = vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkQueueSubmit", result);
        }

        result = vkQueueWaitIdle(queue);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkQueueWaitIdle", result);
        }
}

void copy_buffer_to_buffer(VkDevice device, VkCommandPool command_pool, VkQueue queue, VkBuffer dst_buffer, VkBuffer src_buffer,
                           VkDeviceSize size)
{
        vulkan::CommandBuffer command_buffer(device, command_pool);

        begin_commands(command_buffer);

        //

        VkBufferCopy copy = {};
        // copy.srcOffset = 0;
        // copy.dstOffset = 0;
        copy.size = size;
        vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy);

        //

        end_commands(queue, command_buffer);
}

void copy_buffer_to_image(VkDevice device, VkCommandPool command_pool, VkQueue queue, VkImage image, VkBuffer buffer,
                          uint32_t width, uint32_t height)
{
        vulkan::CommandBuffer command_buffer(device, command_pool);

        begin_commands(command_buffer);

        //

        VkBufferImageCopy region = {};

        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = {0, 0, 0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        //

        end_commands(queue, command_buffer);
}

void cmd_transition_texture_layout(VkCommandBuffer command_buffer, VkImage image, VkImageLayout old_layout,
                                   VkImageLayout new_layout)
{
        VkImageMemoryBarrier barrier = {};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;

        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags source_stage;
        VkPipelineStageFlags destination_stage;

        if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
                 (new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL || new_layout == VK_IMAGE_LAYOUT_GENERAL))
        {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = 0;

                source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destination_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
        else
        {
                error("Unsupported texture layout transition");
        }

        vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void transition_texture_layout(VkDevice device, VkCommandPool command_pool, VkQueue queue, VkImage image,
                               VkImageLayout old_layout, VkImageLayout new_layout)
{
        vulkan::CommandBuffer command_buffer(device, command_pool);

        begin_commands(command_buffer);

        cmd_transition_texture_layout(command_buffer, image, old_layout, new_layout);

        end_commands(queue, command_buffer);
}

void staging_buffer_copy(const vulkan::Device& device, const vulkan::CommandPool& command_pool, const vulkan::Queue& queue,
                         VkBuffer dst_buffer, VkDeviceSize src_data_size, const void* src_data)
{
        ASSERT(command_pool.family_index() == queue.family_index());

        vulkan::Buffer staging_buffer(
                create_buffer(device, src_data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {queue.family_index()}));

        vulkan::DeviceMemory staging_device_memory(create_device_memory(
                device, staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

        //

        copy_host_to_device(staging_device_memory, 0, src_data, src_data_size);

        copy_buffer_to_buffer(device, command_pool, queue, dst_buffer, staging_buffer, src_data_size);
}

template <typename T>
void staging_image_copy(const vulkan::Device& device, const vulkan::CommandPool& graphics_command_pool,
                        const vulkan::Queue& graphics_queue, const vulkan::CommandPool& transfer_command_pool,
                        const vulkan::Queue& transfer_queue, VkImage image, VkImageLayout image_layout, uint32_t width,
                        uint32_t height, const T& pixels)
{
        static_assert(std::is_same_v<typename T::value_type, uint8_t> || std::is_same_v<typename T::value_type, uint16_t> ||
                      std::is_same_v<typename T::value_type, float>);
        static_assert(std::is_same_v<typename T::value_type, std::remove_cvref_t<decltype(pixels[0])>>);

        ASSERT(graphics_command_pool.family_index() == graphics_queue.family_index());
        ASSERT(transfer_command_pool.family_index() == transfer_queue.family_index());

        VkDeviceSize data_size = storage_size(pixels);

        vulkan::Buffer staging_buffer(
                create_buffer(device, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {transfer_queue.family_index()}));

        vulkan::DeviceMemory staging_device_memory(create_device_memory(
                device, staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

        //

        copy_host_to_device(staging_device_memory, 0, pixels.data(), data_size);

        transition_texture_layout(device, graphics_command_pool, graphics_queue, image, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        copy_buffer_to_image(device, transfer_command_pool, transfer_queue, image, staging_buffer, width, height);

        transition_texture_layout(device, graphics_command_pool, graphics_queue, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                  image_layout);
}

vulkan::ImageView create_image_view(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags)
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

        return vulkan::ImageView(device, create_info);
}

template <typename T>
void check_color_buffer_size(const T& pixels, unsigned width, unsigned height)
{
        if (pixels.size() != 4ull * width * height)
        {
                error("Wrong RGBA pixel component count " + to_string(pixels.size()) + " for image dimensions width " +
                      to_string(width) + " and height " + to_string(height));
        }
}
template <typename T>
void check_grayscale_buffer_size(const T& pixels, unsigned width, unsigned height)
{
        if (pixels.size() != 1ull * width * height)
        {
                error("Wrong grayscale pixel component count " + to_string(pixels.size()) + " for image dimensions width " +
                      to_string(width) + " and height " + to_string(height));
        }
}
}

namespace vulkan
{
void BufferWithMemory::write(VkDeviceSize size, const void* data) const
{
        ASSERT(data && host_visible());

        BufferMapper map(*this);
        map.write(0, data, size);
}

void BufferWithMemory::write(const Device& device, const CommandPool& transfer_command_pool, const Queue& transfer_queue,
                             const std::unordered_set<uint32_t>& family_indices, VkDeviceSize size, const void* data) const
{
        ASSERT(data && !host_visible() && usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT));

        if (transfer_command_pool.family_index() != transfer_queue.family_index())
        {
                error("Buffer transfer command pool family index is not equal to transfer queue familty index");
        }
        if (family_indices.count(transfer_queue.family_index()) == 0)
        {
                error("Transfer family index not found in buffer family indices");
        }

        staging_buffer_copy(device, transfer_command_pool, transfer_queue, m_buffer, size, data);
}

BufferWithMemory::BufferWithMemory(BufferMemoryType memory_type, const Device& device,
                                   const std::unordered_set<uint32_t>& family_indices, VkBufferUsageFlags usage,
                                   VkDeviceSize size)
        : m_buffer(create_buffer(device, size, usage, family_indices)),
          m_memory_properties(memory_type == BufferMemoryType::HostVisible ?
                                      (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) :
                                      (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)),
          m_device_memory(create_device_memory(device, m_buffer, m_memory_properties))
{
        ASSERT(size > 0);
}

BufferWithMemory::operator VkBuffer() const
{
        return m_buffer;
}

VkDeviceSize BufferWithMemory::size() const
{
        return m_buffer.size();
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
        return m_memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
}

//

BufferMapper::BufferMapper(const BufferWithMemory& buffer, unsigned long long offset, unsigned long long length)
        : m_device(buffer.m_device_memory.device()), m_device_memory(buffer.m_device_memory), m_length(length)
{
        ASSERT(buffer.memory_properties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        ASSERT(length > 0 && offset + length <= buffer.size());

        VkResult result = vkMapMemory(m_device, m_device_memory, offset, m_length, 0, &m_pointer);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkMapMemory", result);
        }
}

BufferMapper::BufferMapper(const BufferWithMemory& buffer)
        : m_device(buffer.m_device_memory.device()), m_device_memory(buffer.m_device_memory), m_length(buffer.size())
{
        ASSERT(buffer.memory_properties() & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        VkResult result = vkMapMemory(m_device, m_device_memory, 0, VK_WHOLE_SIZE, 0, &m_pointer);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkMapMemory", result);
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
ImageWithMemory::ImageWithMemory(const Device& device, const CommandPool& graphics_command_pool, const Queue& graphics_queue,
                                 const CommandPool& transfer_command_pool, const Queue& transfer_queue,
                                 const std::unordered_set<uint32_t>& family_indices,
                                 const std::vector<VkFormat>& format_candidates, uint32_t width, uint32_t height,
                                 const Span<const std::uint_least8_t>& srgb_pixels)
{
        ASSERT(graphics_command_pool.family_index() == graphics_queue.family_index());
        ASSERT(transfer_command_pool.family_index() == transfer_queue.family_index());

        if (family_indices.count(graphics_queue.family_index()) == 0)
        {
                error("Graphics family index not found in the texture family indices");
        }
        if (family_indices.count(transfer_queue.family_index()) == 0)
        {
                error("Transfer family index not found in the texture family indices");
        }

        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        m_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

        m_format =
                find_supported_2d_image_format(device.physical_device(), format_candidates, tiling, features, m_usage, samples);
        m_image = create_2d_image(device, width, height, m_format, family_indices, samples, tiling, m_usage);
        m_device_memory = create_device_memory(device, m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_image_view = create_image_view(device, m_image, m_format, VK_IMAGE_ASPECT_COLOR_BIT);

        m_width = width;
        m_height = height;

        const VkImageLayout image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
        switch (m_format)
        {
        case VK_FORMAT_R16G16B16A16_UNORM:
        {
                check_color_buffer_size(srgb_pixels, width, height);
                const std::vector<uint16_t> buffer = color_conversion::rgba_pixels_from_srgb_uint8_to_rgb_uint16(srgb_pixels);
                staging_image_copy(device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, m_image,
                                   image_layout, width, height, buffer);
                break;
        }
        case VK_FORMAT_R32G32B32A32_SFLOAT:
        {
                check_color_buffer_size(srgb_pixels, width, height);
                const std::vector<float> buffer = color_conversion::rgba_pixels_from_srgb_uint8_to_rgb_float(srgb_pixels);
                staging_image_copy(device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, m_image,
                                   image_layout, width, height, buffer);
                break;
        }
        case VK_FORMAT_R8G8B8A8_SRGB:
        {
                check_color_buffer_size(srgb_pixels, width, height);
                staging_image_copy(device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, m_image,
                                   image_layout, width, height, srgb_pixels);
                break;
        }
        case VK_FORMAT_R16_UNORM:
        {
                check_grayscale_buffer_size(srgb_pixels, width, height);
                const std::vector<uint16_t> buffer =
                        color_conversion::grayscale_pixels_from_srgb_uint8_to_rgb_uint16(srgb_pixels);
                staging_image_copy(device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, m_image,
                                   image_layout, width, height, buffer);
                break;
        }
        case VK_FORMAT_R32_SFLOAT:
        {
                check_grayscale_buffer_size(srgb_pixels, width, height);
                const std::vector<float> buffer = color_conversion::grayscale_pixels_from_srgb_uint8_to_rgb_float(srgb_pixels);
                staging_image_copy(device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, m_image,
                                   image_layout, width, height, buffer);
                break;
        }
        case VK_FORMAT_R8_SRGB:
        {
                check_grayscale_buffer_size(srgb_pixels, width, height);
                staging_image_copy(device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, m_image,
                                   image_layout, width, height, srgb_pixels);
                break;
        }
        default:
                error("Unsupported texture image format " + format_to_string(m_format));
        }
#pragma GCC diagnostic pop
}

ImageWithMemory::ImageWithMemory(const Device& device, const CommandPool& graphics_command_pool, const Queue& graphics_queue,
                                 const std::unordered_set<uint32_t>& family_indices,
                                 const std::vector<VkFormat>& format_candidates, uint32_t width, uint32_t height)
{
        ASSERT(graphics_command_pool.family_index() == graphics_queue.family_index());

        if (family_indices.count(graphics_queue.family_index()) == 0)
        {
                error("Graphics family index not found in the texture family indices");
        }

        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_TRANSFER_DST_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        m_usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

        m_format =
                find_supported_2d_image_format(device.physical_device(), format_candidates, tiling, features, m_usage, samples);
        m_image = create_2d_image(device, width, height, m_format, family_indices, samples, tiling, m_usage);
        m_device_memory = create_device_memory(device, m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_image_view = create_image_view(device, m_image, m_format, VK_IMAGE_ASPECT_COLOR_BIT);

        m_width = width;
        m_height = height;

        transition_texture_layout(device, graphics_command_pool, graphics_queue, m_image, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

VkImage ImageWithMemory::image() const
{
        return m_image;
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

unsigned ImageWithMemory::width() const
{
        return m_width;
}

unsigned ImageWithMemory::height() const
{
        return m_height;
}

//

DepthAttachment::DepthAttachment(const Device& device, const std::unordered_set<uint32_t>& family_indices,
                                 const std::vector<VkFormat>& formats, VkSampleCountFlagBits samples, uint32_t width,
                                 uint32_t height)
{
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        m_format = find_supported_2d_image_format(device.physical_device(), formats, tiling, features, usage, samples);
        m_image = create_2d_image(device, width, height, m_format, family_indices, samples, tiling, usage);
        m_device_memory = create_device_memory(device, m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_image_view = create_image_view(device, m_image, m_format, VK_IMAGE_ASPECT_DEPTH_BIT);
        m_sample_count = samples;
        m_width = width;
        m_height = height;
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

DepthAttachmentTexture::DepthAttachmentTexture(const Device& device, const std::unordered_set<uint32_t>& family_indices,
                                               const std::vector<VkFormat>& formats, uint32_t width, uint32_t height)
{
        if (width <= 0 || height <= 0)
        {
                error("Depth attachment texture size error");
        }

        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

        m_format = find_supported_2d_image_format(device.physical_device(), formats, tiling, features, usage, samples);

        VkExtent2D max_extent = max_2d_image_extent(device.physical_device(), m_format, tiling, usage);
        m_width = std::min(width, max_extent.width);
        m_height = std::min(height, max_extent.height);

        m_image = create_2d_image(device, m_width, m_height, m_format, family_indices, samples, tiling, usage);
        m_device_memory = create_device_memory(device, m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_image_view = create_image_view(device, m_image, m_format, VK_IMAGE_ASPECT_DEPTH_BIT);
}

VkImage DepthAttachmentTexture::image() const
{
        return m_image;
}

VkFormat DepthAttachmentTexture::format() const
{
        return m_format;
}

VkImageView DepthAttachmentTexture::image_view() const
{
        return m_image_view;
}

unsigned DepthAttachmentTexture::width() const
{
        return m_width;
}

unsigned DepthAttachmentTexture::height() const
{
        return m_height;
}

//

ColorAttachment::ColorAttachment(const Device& device, const std::unordered_set<uint32_t>& family_indices, VkFormat format,
                                 VkSampleCountFlagBits samples, uint32_t width, uint32_t height)
{
        std::vector<VkFormat> candidates = {format}; // должен быть только этот формат
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

        m_format = find_supported_2d_image_format(device.physical_device(), candidates, tiling, features, usage, samples);
        m_image = create_2d_image(device, width, height, m_format, family_indices, samples, tiling, usage);
        m_device_memory = create_device_memory(device, m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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

//

StorageImage::StorageImage(const Device& device, const CommandPool& graphics_command_pool, const Queue& graphics_queue,
                           const std::unordered_set<uint32_t>& family_indices, VkFormat format, uint32_t width, uint32_t height)
{
        ASSERT(graphics_command_pool.family_index() == graphics_queue.family_index());

        if (family_indices.count(graphics_queue.family_index()) == 0)
        {
                error("Graphics family index not found in storage image family indices");
        }

        std::vector<VkFormat> candidates = {format};
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        // Для vkCmdClearColorImage нужно TRANSFER DST
        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;

        m_format = find_supported_2d_image_format(device.physical_device(), candidates, tiling, features, usage, samples);
        m_image = create_2d_image(device, width, height, m_format, family_indices, samples, tiling, usage);
        m_device_memory = create_device_memory(device, m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_image_view = create_image_view(device, m_image, m_format, VK_IMAGE_ASPECT_COLOR_BIT);

        m_width = width;
        m_height = height;

        transition_texture_layout(device, graphics_command_pool, graphics_queue, m_image, VK_IMAGE_LAYOUT_UNDEFINED,
                                  VK_IMAGE_LAYOUT_GENERAL);
}

VkImage StorageImage::image() const
{
        return m_image;
}

VkFormat StorageImage::format() const
{
        return m_format;
}

VkImageView StorageImage::image_view() const
{
        return m_image_view;
}

unsigned StorageImage::width() const
{
        return m_width;
}

unsigned StorageImage::height() const
{
        return m_height;
}

void StorageImage::clear_commands(VkCommandBuffer command_buffer) const
{
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

        barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);

        //

        VkClearColorValue clear_color = clear_color_image_value(m_format);
        VkImageSubresourceRange range = barrier.subresourceRange;

        vkCmdClearColorImage(command_buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &range);

        //

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                             0, nullptr, 1, &barrier);
}
}
