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

#if defined(VULKAN_FOUND)

#include "vertex_buffer.h"

#include "common.h"

#include <cstring>

namespace
{
vulkan::Buffer create_vertex_buffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage)
{
        VkBufferCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size = size;
        create_info.usage = usage;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        return vulkan::Buffer(device, create_info);
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

void buffer_copy(VkDevice device, VkCommandPool command_pool, VkQueue queue, VkBuffer dst_buffer, VkBuffer src_buffer,
                 VkDeviceSize size)
{
        constexpr VkFence NO_FENCE = VK_NULL_HANDLE;

        VkResult result;

        VkCommandBufferAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandPool = command_pool;
        allocate_info.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        result = vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkAllocateCommandBuffers", result);
        }

        try
        {
                VkCommandBufferBeginInfo command_buffer_info = {};
                command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                command_buffer_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                result = vkBeginCommandBuffer(command_buffer, &command_buffer_info);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkBeginCommandBuffer", result);
                }

                VkBufferCopy copy = {};
                // copy.srcOffset = 0;
                // copy.dstOffset = 0;
                copy.size = size;
                vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy);

                result = vkEndCommandBuffer(command_buffer);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkEndCommandBuffer", result);
                }

                VkSubmitInfo submit_info = {};
                submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submit_info.commandBufferCount = 1;
                submit_info.pCommandBuffers = &command_buffer;

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
        catch (...)
        {
                vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
                throw;
        }

        vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}
}

namespace vulkan
{
#if 0
VertexBufferWithHostVisibleMemory::VertexBufferWithHostVisibleMemory(const Device& device, VkDeviceSize data_size,
                                                                     const void* data)
        : m_vertex_buffer(create_vertex_buffer(device, data_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)),
          m_vertex_device_memory(create_device_memory(device, m_vertex_buffer,
                                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
{
        memory_copy(device, m_vertex_device_memory, data, data_size);
}

VertexBufferWithHostVisibleMemory::operator VkBuffer() const noexcept
{
        return m_vertex_buffer;
}
#endif

//

VertexBufferWithDeviceLocalMemory::VertexBufferWithDeviceLocalMemory(const Device& device, VkCommandPool command_pool,
                                                                     VkQueue queue, VkDeviceSize data_size, const void* data)
        : m_vertex_buffer(
                  create_vertex_buffer(device, data_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)),
          m_vertex_device_memory(create_device_memory(device, m_vertex_buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
{
        Buffer staging_buffer(create_vertex_buffer(device, data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT));
        DeviceMemory staging_device_memory(create_device_memory(
                device, staging_buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT));

        memory_copy(device, staging_device_memory, data, data_size);

        buffer_copy(device, command_pool, queue, m_vertex_buffer, staging_buffer, data_size);
}

VertexBufferWithDeviceLocalMemory::operator VkBuffer() const noexcept
{
        return m_vertex_buffer;
}
}

#endif
