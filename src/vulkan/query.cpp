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

#include "query.h"

#include "error.h"
#include "print.h"

#include <src/com/error.h>
#include <src/com/print.h>

#include <algorithm>
#include <sstream>

namespace ns::vulkan
{
namespace
{
std::string vulkan_formats_to_string(const std::vector<VkFormat>& formats)
{
        if (formats.empty())
        {
                return std::string();
        }
        std::string s = vulkan::format_to_string(formats[0]);
        for (std::size_t i = 1; i < formats.size(); ++i)
        {
                s += ", ";
                s += vulkan::format_to_string(formats[i]);
        }
        return s;
}
}

std::unordered_set<std::string> supported_instance_extensions()
{
        uint32_t extension_count;
        VkResult result;

        result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateInstanceExtensionProperties", result);
        }

        if (extension_count < 1)
        {
                return {};
        }

        std::vector<VkExtensionProperties> extensions(extension_count);

        result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateInstanceExtensionProperties", result);
        }

        std::unordered_set<std::string> extension_set;

        for (const VkExtensionProperties& e : extensions)
        {
                extension_set.emplace(e.extensionName);
        }

        return extension_set;
}

std::unordered_set<std::string> supported_validation_layers()
{
        uint32_t layer_count;
        VkResult result;

        result = vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateInstanceLayerProperties", result);
        }

        if (layer_count < 1)
        {
                return {};
        }

        std::vector<VkLayerProperties> layers(layer_count);

        result = vkEnumerateInstanceLayerProperties(&layer_count, layers.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateInstanceLayerProperties", result);
        }

        std::unordered_set<std::string> layer_set;

        for (const VkLayerProperties& l : layers)
        {
                layer_set.emplace(l.layerName);
        }

        return layer_set;
}

uint32_t supported_instance_api_version()
{
        PFN_vkEnumerateInstanceVersion f = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
                vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));

        if (!f)
        {
                return VK_MAKE_VERSION(1, 0, 0);
        }

        uint32_t api_version;
        VkResult result = f(&api_version);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateInstanceVersion", result);
        }

        return api_version;
}

void check_instance_extension_support(const std::vector<std::string>& required_extensions)
{
        if (required_extensions.empty())
        {
                return;
        }

        const std::unordered_set<std::string> extension_set = supported_instance_extensions();

        for (const std::string& e : required_extensions)
        {
                if (extension_set.count(e) < 1)
                {
                        error("Vulkan instance extension " + e + " is not supported");
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

        for (const std::string& l : required_layers)
        {
                if (layer_set.count(l) < 1)
                {
                        error("Vulkan validation layer " + l + " is not supported");
                }
        }
}

void check_api_version(uint32_t required_api_version)
{
        uint32_t api_version = supported_instance_api_version();

        if (required_api_version > api_version)
        {
                error("Vulkan API version " + api_version_to_string(required_api_version)
                      + " is not supported. Supported " + api_version_to_string(api_version) + ".");
        }
}

VkFormat find_supported_format(
        VkPhysicalDevice physical_device,
        const std::vector<VkFormat>& candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features)
{
        if (tiling == VK_IMAGE_TILING_OPTIMAL)
        {
                for (VkFormat format : candidates)
                {
                        VkFormatProperties properties;
                        vkGetPhysicalDeviceFormatProperties(physical_device, format, &properties);
                        if ((properties.optimalTilingFeatures & features) == features)
                        {
                                return format;
                        }
                }
        }
        else if (tiling == VK_IMAGE_TILING_LINEAR)
        {
                for (VkFormat format : candidates)
                {
                        VkFormatProperties properties;
                        vkGetPhysicalDeviceFormatProperties(physical_device, format, &properties);
                        if ((properties.linearTilingFeatures & features) == features)
                        {
                                return format;
                        }
                }
        }
        else
        {
                error("Unknown image tiling " + to_string(static_cast<long long>(tiling)));
        }

        std::ostringstream oss;

        oss << "Failed to find supported 2D image format.";
        oss << " Format candidates " << vulkan_formats_to_string(candidates) << ".";
        oss << " Tiling " << static_cast<long long>(tiling) << ".";
        oss << std::hex;
        oss << " Features 0x" << features << ".";

        error(oss.str());
}

VkFormat find_supported_image_format(
        VkPhysicalDevice physical_device,
        const std::vector<VkFormat>& candidates,
        VkImageType image_type,
        VkImageTiling tiling,
        VkFormatFeatureFlags features,
        VkImageUsageFlags usage,
        VkSampleCountFlags sample_count)
{
        for (VkFormat format : candidates)
        {
                VkFormatProperties properties;
                vkGetPhysicalDeviceFormatProperties(physical_device, format, &properties);

                if (tiling == VK_IMAGE_TILING_OPTIMAL)
                {
                        if ((properties.optimalTilingFeatures & features) != features)
                        {
                                continue;
                        }
                }
                else if (tiling == VK_IMAGE_TILING_LINEAR)
                {
                        if ((properties.linearTilingFeatures & features) != features)
                        {
                                continue;
                        }
                }
                else
                {
                        error("Unknown image tiling " + to_string(static_cast<long long>(tiling)));
                }

                VkImageFormatProperties image_properties;
                VkResult result = vkGetPhysicalDeviceImageFormatProperties(
                        physical_device, format, image_type, tiling, usage, 0 /*VkImageCreateFlags*/,
                        &image_properties);
                if (result != VK_SUCCESS)
                {
                        vulkan_function_error("vkGetPhysicalDeviceImageFormatProperties", result);
                }

                if ((image_properties.sampleCounts & sample_count) != sample_count)
                {
                        continue;
                }

                return format;
        }

        std::ostringstream oss;

        oss << "Failed to find supported image format.";
        oss << " Format candidates " << vulkan_formats_to_string(candidates) << ".";
        oss << " Image type " << image_type_to_string(image_type) << ".";
        oss << " Tiling " << static_cast<long long>(tiling) << ".";
        oss << std::hex;
        oss << " Features 0x" << features << ".";
        oss << " Usage 0x" << usage << ".";
        oss << " Sample count 0x" << sample_count << ".";

        error(oss.str());
}

VkExtent3D max_image_extent(
        VkPhysicalDevice physical_device,
        VkFormat format,
        VkImageType image_type,
        VkImageTiling tiling,
        VkImageUsageFlags usage)
{
        VkImageFormatProperties image_properties;

        VkResult result = vkGetPhysicalDeviceImageFormatProperties(
                physical_device, format, image_type, tiling, usage, 0 /*VkImageCreateFlags*/, &image_properties);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkGetPhysicalDeviceImageFormatProperties", result);
        }

        return image_properties.maxExtent;
}

VkSampleCountFlagBits supported_color_depth_framebuffer_sample_count_flag(
        VkPhysicalDevice physical_device,
        int required_minimum_sample_count)
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

int sample_count_flag_to_integer(VkSampleCountFlagBits sample_count)
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

        static_assert(sizeof(sample_count) <= sizeof(long long));
        error("Unknown sample count flag " + to_string(static_cast<long long>(sample_count)));
}

uint32_t physical_device_memory_type_index(
        VkPhysicalDevice physical_device,
        uint32_t memory_type_bits,
        VkMemoryPropertyFlags memory_property_flags)
{
        ASSERT(physical_device != VK_NULL_HANDLE);

        VkPhysicalDeviceMemoryProperties memory_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

        if (memory_properties.memoryTypeCount >= static_cast<unsigned>(Limits<uint32_t>::digits()))
        {
                error("memoryTypeCount >= memory_type_bits bit count");
        }

        for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
        {
                if ((memory_type_bits & (static_cast<uint32_t>(1) << i))
                    && (memory_properties.memoryTypes[i].propertyFlags & memory_property_flags)
                               == memory_property_flags)
                {
                        return i;
                }
        }

        error("Failed to find suitable memory type");
}
}
