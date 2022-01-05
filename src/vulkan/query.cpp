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

#include "query.h"

#include "error.h"
#include "print.h"
#include "settings.h"

#include <src/com/enum.h>
#include <src/com/error.h>
#include <src/com/print.h>

#include <algorithm>

namespace ns::vulkan
{
namespace
{
std::uint32_t find_extension_count()
{
        std::uint32_t extension_count;
        VULKAN_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr));
        return extension_count;
}

std::uint32_t find_layer_count()
{
        std::uint32_t layer_count;
        VULKAN_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
        return layer_count;
}
}

std::unordered_set<std::string> supported_instance_extensions()
{
        std::uint32_t extension_count = find_extension_count();
        if (extension_count < 1)
        {
                return {};
        }

        std::vector<VkExtensionProperties> extensions(extension_count);
        VULKAN_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data()));

        std::unordered_set<std::string> extension_set;
        for (const VkExtensionProperties& e : extensions)
        {
                extension_set.emplace(e.extensionName);
        }
        return extension_set;
}

std::unordered_set<std::string> supported_validation_layers()
{
        std::uint32_t layer_count = find_layer_count();
        if (layer_count < 1)
        {
                return {};
        }

        std::vector<VkLayerProperties> layers(layer_count);
        VULKAN_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, layers.data()));

        std::unordered_set<std::string> layer_set;
        for (const VkLayerProperties& l : layers)
        {
                layer_set.emplace(l.layerName);
        }
        return layer_set;
}

std::uint32_t supported_instance_api_version()
{
        const PFN_vkEnumerateInstanceVersion enumerate_instance_version =
                reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
                        vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));

        if (!enumerate_instance_version)
        {
                return VK_MAKE_API_VERSION(0, 1, 0, 0);
        }

        std::uint32_t api_version;
        VULKAN_CHECK(enumerate_instance_version(&api_version));

        return api_version;
}

void check_instance_extension_support(const std::vector<std::string>& required_extensions)
{
        if (required_extensions.empty())
        {
                return;
        }

        const std::unordered_set<std::string> extension_set = supported_instance_extensions();

        for (const std::string& extension : required_extensions)
        {
                if (!extension_set.contains(extension))
                {
                        error("Vulkan instance extension " + extension + " is not supported");
                }
        }
}

void check_validation_layer_support(const std::vector<std::string>& required_layers)
{
        if (required_layers.empty())
        {
                return;
        }

        const std::unordered_set<std::string> layer_set = supported_validation_layers();

        for (const std::string& layer : required_layers)
        {
                if (!layer_set.contains(layer))
                {
                        error("Vulkan validation layer " + layer + " is not supported");
                }
        }
}

void check_instance_api_version()
{
        const std::uint32_t api_version = supported_instance_api_version();

        if (!api_version_suitable(api_version))
        {
                error("Vulkan instance API version " + api_version_to_string(API_VERSION)
                      + " is not supported. Supported " + api_version_to_string(api_version) + ".");
        }
}

VkExtent3D find_max_image_extent(
        const VkPhysicalDevice physical_device,
        const VkFormat format,
        const VkImageType image_type,
        const VkImageTiling tiling,
        const VkImageUsageFlags usage)
{
        VkImageFormatProperties image_properties;
        VULKAN_CHECK(vkGetPhysicalDeviceImageFormatProperties(
                physical_device, format, image_type, tiling, usage, 0 /*VkImageCreateFlags*/, &image_properties));
        return image_properties.maxExtent;
}

VkSampleCountFlagBits supported_color_depth_framebuffer_sample_count_flag(
        const VkPhysicalDevice physical_device,
        const int required_minimum_sample_count)
{
        constexpr int MIN_SAMPLE_COUNT = 1;
        constexpr int MAX_SAMPLE_COUNT = 64;

        if (required_minimum_sample_count < MIN_SAMPLE_COUNT)
        {
                error("The required minimum sample count " + to_string(required_minimum_sample_count) + " is less than "
                      + to_string(MIN_SAMPLE_COUNT));
        }
        if (required_minimum_sample_count > MAX_SAMPLE_COUNT)
        {
                error("The required minimum sample count " + to_string(required_minimum_sample_count)
                      + " is greater than " + to_string(MAX_SAMPLE_COUNT));
        }

        const auto set = [](const VkSampleCountFlags flags, const VkSampleCountFlagBits bits)
        {
                return (flags & bits) == bits;
        };

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_device, &properties);

        const VkSampleCountFlags flags =
                properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;

        if ((required_minimum_sample_count <= 1) && set(flags, VK_SAMPLE_COUNT_1_BIT))
        {
                return VK_SAMPLE_COUNT_1_BIT;
        }
        if ((required_minimum_sample_count <= 2) && set(flags, VK_SAMPLE_COUNT_2_BIT))
        {
                return VK_SAMPLE_COUNT_2_BIT;
        }
        if ((required_minimum_sample_count <= 4) && set(flags, VK_SAMPLE_COUNT_4_BIT))
        {
                return VK_SAMPLE_COUNT_4_BIT;
        }
        if ((required_minimum_sample_count <= 8) && set(flags, VK_SAMPLE_COUNT_8_BIT))
        {
                return VK_SAMPLE_COUNT_8_BIT;
        }
        if ((required_minimum_sample_count <= 16) && set(flags, VK_SAMPLE_COUNT_16_BIT))
        {
                return VK_SAMPLE_COUNT_16_BIT;
        }
        if ((required_minimum_sample_count <= 32) && set(flags, VK_SAMPLE_COUNT_32_BIT))
        {
                return VK_SAMPLE_COUNT_32_BIT;
        }
        if ((required_minimum_sample_count <= 64) && set(flags, VK_SAMPLE_COUNT_64_BIT))
        {
                return VK_SAMPLE_COUNT_64_BIT;
        }

        error("The required minimum sample count " + to_string(required_minimum_sample_count) + " is not available");
}

int sample_count_flag_to_integer(const VkSampleCountFlagBits sample_count)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (sample_count)
        {
        case VK_SAMPLE_COUNT_1_BIT:
                return 1;
        case VK_SAMPLE_COUNT_2_BIT:
                return 2;
        case VK_SAMPLE_COUNT_4_BIT:
                return 4;
        case VK_SAMPLE_COUNT_8_BIT:
                return 8;
        case VK_SAMPLE_COUNT_16_BIT:
                return 16;
        case VK_SAMPLE_COUNT_32_BIT:
                return 32;
        case VK_SAMPLE_COUNT_64_BIT:
                return 64;
        }
#pragma GCC diagnostic pop

        error("Unknown sample count flag " + to_string(enum_to_int(sample_count)));
}

std::uint32_t physical_device_memory_type_index(
        const VkPhysicalDevice physical_device,
        const std::uint32_t memory_type_bits,
        const VkMemoryPropertyFlags memory_property_flags)
{
        ASSERT(physical_device != VK_NULL_HANDLE);

        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

        if (memory_properties.memoryTypeCount >= static_cast<unsigned>(Limits<std::uint32_t>::digits()))
        {
                error("memoryTypeCount >= memory_type_bits bit count");
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
