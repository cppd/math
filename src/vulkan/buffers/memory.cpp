/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../error.h"
#include "../objects.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>

#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace ns::vulkan
{
namespace
{
std::uint32_t physical_device_memory_type_index(
        const VkPhysicalDevice physical_device,
        const std::uint32_t memory_type_bits,
        const VkMemoryPropertyFlags memory_property_flags)
{
        ASSERT(physical_device != VK_NULL_HANDLE);

        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

        if (memory_properties.memoryTypeCount > static_cast<unsigned>(Limits<std::uint32_t>::digits()))
        {
                error("memoryTypeCount (" + to_string(memory_properties.memoryTypeCount) + ") > "
                      + to_string(Limits<std::uint32_t>::digits()));
        }

        for (std::uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
        {
                if ((memory_type_bits & (static_cast<std::uint32_t>(1) << i))
                    && (memory_properties.memoryTypes[i].propertyFlags & memory_property_flags)
                               == memory_property_flags)
                {
                        return i;
                }
        }

        error("Failed to find suitable memory type");
}
}

handle::DeviceMemory create_device_memory(
        const VkDevice device,
        const VkPhysicalDevice physical_device,
        const VkBuffer buffer,
        const VkMemoryPropertyFlags properties,
        const VkMemoryAllocateFlags allocate_flags)
{
        VkMemoryRequirements memory_requirements;
        vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

        VkMemoryAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize = memory_requirements.size;
        allocate_info.memoryTypeIndex =
                physical_device_memory_type_index(physical_device, memory_requirements.memoryTypeBits, properties);

        VkMemoryAllocateFlagsInfo allocate_flags_info;

        if (allocate_flags)
        {
                if ((allocate_flags & VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT) == VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT)
                {
                        error("VK_MEMORY_ALLOCATE_DEVICE_MASK_BIT is not supported");
                }
                allocate_flags_info = {};
                allocate_flags_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
                allocate_flags_info.flags = allocate_flags;
                allocate_info.pNext = &allocate_flags_info;
        }

        handle::DeviceMemory device_memory(device, allocate_info);

        VULKAN_CHECK(vkBindBufferMemory(device, buffer, device_memory, 0));

        return device_memory;
}

handle::DeviceMemory create_device_memory(
        const VkDevice device,
        const VkPhysicalDevice physical_device,
        const VkImage image,
        const VkMemoryPropertyFlags properties)
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
