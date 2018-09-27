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

#include "physical_device.h"

#include "common.h"
#include "query.h"

#include "com/error.h"

#include <algorithm>

namespace
{
bool find_graphics_family(const std::vector<VkQueueFamilyProperties>& queue_families, uint32_t* family_index)
{
        for (uint32_t i = 0; i < queue_families.size(); ++i)
        {
                const VkQueueFamilyProperties& p = queue_families[i];

                if (p.queueCount >= 1 && (p.queueFlags & VK_QUEUE_GRAPHICS_BIT))
                {
                        *family_index = i;
                        return true;
                }
        }
        return false;
}

bool find_compute_family(const std::vector<VkQueueFamilyProperties>& queue_families, uint32_t* family_index)
{
        for (uint32_t i = 0; i < queue_families.size(); ++i)
        {
                const VkQueueFamilyProperties& p = queue_families[i];

                if (p.queueCount >= 1 && (p.queueFlags & VK_QUEUE_COMPUTE_BIT))
                {
                        *family_index = i;
                        return true;
                }
        }
        return false;
}

bool find_transfer_family(const std::vector<VkQueueFamilyProperties>& queue_families, uint32_t* family_index)
{
        for (uint32_t i = 0; i < queue_families.size(); ++i)
        {
                const VkQueueFamilyProperties& p = queue_families[i];

                if (p.queueCount >= 1 && (p.queueFlags & VK_QUEUE_TRANSFER_BIT))
                {
                        if (!(p.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(p.queueFlags & VK_QUEUE_COMPUTE_BIT))
                        {
                                *family_index = i;
                                return true;
                        }
                }
        }
        return false;
}

bool find_presentation_family(VkSurfaceKHR surface, VkPhysicalDevice device,
                              const std::vector<VkQueueFamilyProperties>& queue_families, uint32_t* family_index)
{
        for (uint32_t i = 0; i < queue_families.size(); ++i)
        {
                if (queue_families[i].queueCount >= 1)
                {
                        VkBool32 presentation_support;

                        VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentation_support);
                        if (result != VK_SUCCESS)
                        {
                                vulkan::vulkan_function_error("vkGetPhysicalDeviceSurfaceSupportKHR", result);
                        }

                        if (presentation_support == VK_TRUE)
                        {
                                *family_index = i;
                                return true;
                        }
                }
        }
        return false;
}

bool physical_device_features_are_supported(const std::vector<vulkan::PhysicalDeviceFeatures>& required_features,
                                            VkPhysicalDeviceFeatures device_features)
{
        for (vulkan::PhysicalDeviceFeatures f : required_features)
        {
                switch (f)
                {
                case vulkan::PhysicalDeviceFeatures::GeometryShader:
                        if (!device_features.geometryShader)
                        {
                                return false;
                        }
                        break;
                case vulkan::PhysicalDeviceFeatures::SampleRateShading:
                        if (!device_features.sampleRateShading)
                        {
                                return false;
                        }
                        break;
                case vulkan::PhysicalDeviceFeatures::SamplerAnisotropy:
                        if (!device_features.samplerAnisotropy)
                        {
                                return false;
                        }
                        break;
                case vulkan::PhysicalDeviceFeatures::TessellationShader:
                        if (!device_features.tessellationShader)
                        {
                                return false;
                        }
                        break;
                }
        }

        return true;
}

void check_no_intersection(std::vector<vulkan::PhysicalDeviceFeatures> required_features,
                           std::vector<vulkan::PhysicalDeviceFeatures> optional_features)
{
        std::sort(required_features.begin(), required_features.end());
        std::sort(optional_features.begin(), optional_features.end());

        std::vector<vulkan::PhysicalDeviceFeatures> intersection;

        std::set_intersection(required_features.cbegin(), required_features.cend(), optional_features.cbegin(),
                              optional_features.cend(), std::back_inserter(intersection));

        if (!intersection.empty())
        {
                error("Required and optional physical device features intersection");
        }
}
}

namespace vulkan
{
VkPhysicalDeviceFeatures make_enabled_device_features(const std::vector<PhysicalDeviceFeatures>& required_features,
                                                      const std::vector<PhysicalDeviceFeatures>& optional_features,
                                                      const VkPhysicalDeviceFeatures& supported_device_features)
{
        check_no_intersection(required_features, optional_features);

        VkPhysicalDeviceFeatures device_features = {};

        for (vulkan::PhysicalDeviceFeatures f : required_features)
        {
                switch (f)
                {
                case vulkan::PhysicalDeviceFeatures::GeometryShader:
                        if (!supported_device_features.geometryShader)
                        {
                                error("Required physical device feature Geometry Shader is not supported");
                        }
                        device_features.geometryShader = true;
                        break;
                case vulkan::PhysicalDeviceFeatures::SampleRateShading:
                        if (!supported_device_features.sampleRateShading)
                        {
                                error("Required physical device feature Sample Rate Shading is not supported");
                        }
                        device_features.sampleRateShading = true;
                        break;
                case vulkan::PhysicalDeviceFeatures::SamplerAnisotropy:
                        if (!supported_device_features.samplerAnisotropy)
                        {
                                error("Required physical device feature Sampler Anisotropy is not supported");
                        }
                        device_features.samplerAnisotropy = true;
                        break;
                case vulkan::PhysicalDeviceFeatures::TessellationShader:
                        if (!supported_device_features.tessellationShader)
                        {
                                error("Required physical device feature Tessellation Shader is not supported");
                        }
                        device_features.tessellationShader = true;
                        break;
                }
        }

        for (vulkan::PhysicalDeviceFeatures f : optional_features)
        {
                switch (f)
                {
                case vulkan::PhysicalDeviceFeatures::GeometryShader:
                        device_features.geometryShader = supported_device_features.geometryShader;
                        break;
                case vulkan::PhysicalDeviceFeatures::SampleRateShading:
                        device_features.sampleRateShading = supported_device_features.sampleRateShading;
                        break;
                case vulkan::PhysicalDeviceFeatures::SamplerAnisotropy:
                        device_features.samplerAnisotropy = supported_device_features.samplerAnisotropy;
                        break;
                case vulkan::PhysicalDeviceFeatures::TessellationShader:
                        device_features.tessellationShader = supported_device_features.tessellationShader;
                        break;
                }
        }

        return device_features;
}

bool find_swap_chain_details(VkSurfaceKHR surface, VkPhysicalDevice device, SwapChainDetails* swap_chain_details)
{
        VkSurfaceCapabilitiesKHR surface_capabilities;

        VkResult result;
        result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surface_capabilities);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkGetPhysicalDeviceSurfaceCapabilitiesKHR", result);
        }

        std::vector<VkSurfaceFormatKHR> surface_formats = vulkan::surface_formats(device, surface);
        if (surface_formats.empty())
        {
                return false;
        }

        std::vector<VkPresentModeKHR> present_modes = vulkan::present_modes(device, surface);
        if (present_modes.empty())
        {
                return false;
        }

        if (swap_chain_details)
        {
                swap_chain_details->surface_capabilities = surface_capabilities;
                swap_chain_details->surface_formats = surface_formats;
                swap_chain_details->present_modes = present_modes;
        }

        return true;
}

PhysicalDevice find_physical_device(VkInstance instance, VkSurfaceKHR surface, int api_version_major, int api_version_minor,
                                    const std::vector<std::string>& required_extensions,
                                    const std::vector<PhysicalDeviceFeatures>& required_features)
{
        const uint32_t required_api_version = VK_MAKE_VERSION(api_version_major, api_version_minor, 0);

        for (const VkPhysicalDevice& device : vulkan::physical_devices(instance))
        {
                ASSERT(device != VK_NULL_HANDLE);

                VkPhysicalDeviceProperties properties;
                VkPhysicalDeviceFeatures features;

                vkGetPhysicalDeviceProperties(device, &properties);
                vkGetPhysicalDeviceFeatures(device, &features);

                if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                    properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &&
                    properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU &&
                    properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU)
                {
                        continue;
                }

                if (!physical_device_features_are_supported(required_features, features))
                {
                        continue;
                }

                if (required_api_version > properties.apiVersion)
                {
                        continue;
                }

                if (!vulkan::device_supports_extensions(device, required_extensions))
                {
                        continue;
                }

                if (!find_swap_chain_details(surface, device, nullptr))
                {
                        continue;
                }

                const std::vector<VkQueueFamilyProperties> families = vulkan::queue_families(device);
                uint32_t graphics, compute, transfer, presentation;
                if (!find_graphics_family(families, &graphics))
                {
                        continue;
                }
                if (!find_compute_family(families, &compute))
                {
                        continue;
                }
                if (!find_presentation_family(surface, device, families, &presentation))
                {
                        continue;
                }
                if (!find_transfer_family(families, &transfer))
                {
                        continue;
                }

                return PhysicalDevice(device, graphics, compute, transfer, presentation, features);
        }

        error("Failed to find a suitable Vulkan physical device");
}
}
