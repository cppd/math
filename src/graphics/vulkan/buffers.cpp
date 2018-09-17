/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "common.h"
#include "query.h"

#include "com/alg.h"
#include "com/error.h"
#include "com/print.h"

#include <cstring>

namespace
{
vulkan::Buffer create_buffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage,
                             const std::vector<uint32_t>& family_indices)
{
        ASSERT(size > 0);

        VkBufferCreateInfo create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = size;
        create_info.usage = usage;

        ASSERT(family_indices.size() == unique_elements(family_indices).size());

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

        return vulkan::Buffer(device, create_info);
}

vulkan::Image create_image(VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                           VkImageUsageFlags usage, const std::vector<uint32_t>& family_indices)
{
        ASSERT(width > 0 && height > 0);

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
        create_info.samples = VK_SAMPLE_COUNT_1_BIT;
        // create_info.flags = 0;

        ASSERT(family_indices.size() == unique_elements(family_indices).size());

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

        return vulkan::Image(device, create_info);
}

vulkan::DeviceMemory create_device_memory(const vulkan::Device& device, VkBuffer buffer, VkMemoryPropertyFlags properties)
{
        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

        VkMemoryAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex = device.physical_device_memory_type_index(memory_requirements.memoryTypeBits, properties);

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
        allocate_info.memoryTypeIndex = device.physical_device_memory_type_index(memory_requirements.memoryTypeBits, properties);

        vulkan::DeviceMemory device_memory(device, allocate_info);

        VkResult result = vkBindImageMemory(device, image, device_memory, 0);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkBindImageMemory", result);
        }

        return device_memory;
}

void memory_copy(VkDevice device, VkDeviceMemory device_memory, const void* data, VkDeviceSize data_size)
{
        void* map_memory_data;

        VkResult result = vkMapMemory(device, device_memory, 0, data_size, 0, &map_memory_data);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkMapMemory", result);
        }

        std::memcpy(map_memory_data, data, data_size);

        vkUnmapMemory(device, device_memory);

        // vkFlushMappedMemoryRanges, vkInvalidateMappedMemoryRanges
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
                vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
        }
}

void end_commands(VkQueue queue, const vulkan::CommandBuffer& command_buffer)
{
        constexpr VkFence NO_FENCE = VK_NULL_HANDLE;

        VkResult result;

        result = vkEndCommandBuffer(command_buffer);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkEndCommandBuffer", result);
        }

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers = command_buffer.data();

        result = vkQueueSubmit(queue, 1, &submit_info, NO_FENCE);
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

void transition_image_layout(VkDevice device, VkCommandPool command_pool, VkQueue queue, VkImage image, VkFormat format,
                             VkImageLayout old_layout, VkImageLayout new_layout)
{
        vulkan::CommandBuffer command_buffer(device, command_pool);

        begin_commands(command_buffer);

        //

        VkImageMemoryBarrier barrier = {};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;

        if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
                if (format == VK_FORMAT_D32_SFLOAT)
                {
                        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                }
                else if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
                {
                        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                }
                else
                {
                        error("Unsupported image format for layout transition");
                }
        }
        else if (new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL || new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }
        else
        {
                error("Unsupported new layout for image layout transition");
        }

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
        else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask =
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        else
        {
                error("Unsupported image layout transition");
        }

        vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        //

        end_commands(queue, command_buffer);
}

void staging_buffer_copy(const vulkan::Device& device, VkCommandPool command_pool, VkQueue queue, VkBuffer dst_buffer,
                         VkDeviceSize src_data_size, const void* src_data)
{
        vulkan::Buffer staging_buffer(create_buffer(device, src_data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {}));

        vulkan::DeviceMemory staging_device_memory(create_device_memory(
                device, staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

        //

        memory_copy(device, staging_device_memory, src_data, src_data_size);

        copy_buffer_to_buffer(device, command_pool, queue, dst_buffer, staging_buffer, src_data_size);
}

void staging_image_copy(const vulkan::Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                        VkCommandPool transfer_command_pool, VkQueue transfer_queue, VkImage image, VkFormat image_format,
                        VkImageLayout image_layout, uint32_t width, uint32_t height,
                        const std::vector<unsigned char>& rgba_pixels)
{
        VkDeviceSize data_size = width * height * 4;

        if (rgba_pixels.size() != data_size)
        {
                error("Wrong RGBA pixel component count " + to_string(rgba_pixels.size()) + " for image dimensions width " +
                      to_string(width) + " and height " + to_string(height));
        }

        vulkan::Buffer staging_buffer(create_buffer(device, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {}));

        vulkan::DeviceMemory staging_device_memory(create_device_memory(
                device, staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

        //

        memory_copy(device, staging_device_memory, rgba_pixels.data(), data_size);

        transition_image_layout(device, graphics_command_pool, graphics_queue, image, image_format, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        copy_buffer_to_image(device, transfer_command_pool, transfer_queue, image, staging_buffer, width, height);

        transition_image_layout(device, graphics_command_pool, graphics_queue, image, image_format,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image_layout);
}
}

namespace vulkan
{
#if 0
VertexBufferWithHostVisibleMemory::VertexBufferWithHostVisibleMemory(const Device& device, VkDeviceSize data_size,
                                                                     const void* data)
        : m_buffer(create_buffer(device, data_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, {})),
          m_device_memory(create_device_memory(device, m_buffer,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
{
        memory_copy(device, m_device_memory, data, data_size);
}

VertexBufferWithHostVisibleMemory::operator VkBuffer() const noexcept
{
        return m_buffer;
}
#endif

//

VertexBufferWithDeviceLocalMemory::VertexBufferWithDeviceLocalMemory(const Device& device, VkCommandPool command_pool,
                                                                     VkQueue queue, const std::vector<uint32_t>& family_indices,
                                                                     VkDeviceSize data_size, const void* data)
        : m_buffer(create_buffer(device, data_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 family_indices)),
          m_device_memory(create_device_memory(device, m_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
{
        ASSERT(family_indices.size() > 0);

        staging_buffer_copy(device, command_pool, queue, m_buffer, data_size, data);
}

VertexBufferWithDeviceLocalMemory::operator VkBuffer() const noexcept
{
        return m_buffer;
}

//

IndexBufferWithDeviceLocalMemory::IndexBufferWithDeviceLocalMemory(const Device& device, VkCommandPool command_pool,
                                                                   VkQueue queue, const std::vector<uint32_t>& family_indices,
                                                                   VkDeviceSize data_size, const void* data)
        : m_buffer(create_buffer(device, data_size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                 family_indices)),
          m_device_memory(create_device_memory(device, m_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
{
        ASSERT(family_indices.size() > 0);

        staging_buffer_copy(device, command_pool, queue, m_buffer, data_size, data);
}

IndexBufferWithDeviceLocalMemory::operator VkBuffer() const noexcept
{
        return m_buffer;
}

//

UniformBufferWithHostVisibleMemory::UniformBufferWithHostVisibleMemory(const Device& device, VkDeviceSize data_size)
        : m_device(device),
          m_data_size(data_size),
          m_buffer(create_buffer(device, data_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, {})),
          m_device_memory(create_device_memory(device, m_buffer,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
{
}

UniformBufferWithHostVisibleMemory::operator VkBuffer() const noexcept
{
        return m_buffer;
}

VkDeviceSize UniformBufferWithHostVisibleMemory::size() const noexcept
{
        return m_data_size;
}

void UniformBufferWithHostVisibleMemory::copy(const void* data) const
{
        memory_copy(m_device, m_device_memory, data, m_data_size);
}

//

TextureImage::TextureImage(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                           VkCommandPool transfer_command_pool, VkQueue transfer_queue,
                           const std::vector<uint32_t>& family_indices, uint32_t width, uint32_t height,
                           const std::vector<unsigned char>& rgba_pixels)
        : m_image(create_image(device, width, height, IMAGE_FORMAT, VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, family_indices)),
          m_device_memory(create_device_memory(device, m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
{
        ASSERT(family_indices.size() > 0);

        staging_image_copy(device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, m_image,
                           IMAGE_FORMAT, IMAGE_LAYOUT, width, height, rgba_pixels);
}

TextureImage::operator VkImage() const noexcept
{
        return m_image;
}

VkFormat TextureImage::image_format() const noexcept
{
        return IMAGE_FORMAT;
}

VkImageLayout TextureImage::image_layout() const noexcept
{
        return IMAGE_LAYOUT;
}

//

DepthImage::DepthImage(const Device& device, VkCommandPool graphics_command_pool, VkQueue graphics_queue,
                       const std::vector<uint32_t>& family_indices, uint32_t width, uint32_t height)
        : m_image_format(find_supported_format(device.physical_device(),
                                               {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                               VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)),
          m_image(create_image(device, width, height, m_image_format, VK_IMAGE_TILING_OPTIMAL,
                               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, family_indices)),
          m_device_memory(create_device_memory(device, m_image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
{
        ASSERT(family_indices.size() > 0);

        transition_image_layout(device, graphics_command_pool, graphics_queue, m_image, m_image_format, VK_IMAGE_LAYOUT_UNDEFINED,
                                IMAGE_LAYOUT);
}

DepthImage::operator VkImage() const noexcept
{
        return m_image;
}

VkFormat DepthImage::image_format() const noexcept
{
        return m_image_format;
}

VkImageLayout DepthImage::image_layout() const noexcept
{
        return IMAGE_LAYOUT;
}
}
