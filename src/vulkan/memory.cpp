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

#include "memory.h"

#include "error.h"
#include "query.h"

namespace ns::vulkan
{
handle::DeviceMemory create_device_memory(
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

        handle::DeviceMemory device_memory(device, allocate_info);

        VULKAN_CHECK(vkBindBufferMemory(device, buffer, device_memory, 0));

        return device_memory;
}

handle::DeviceMemory create_device_memory(
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

        handle::DeviceMemory device_memory(device, allocate_info);

        VULKAN_CHECK(vkBindImageMemory(device, image, device_memory, 0));

        return device_memory;
}
}
