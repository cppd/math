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

#include "device.h"

#include "error.h"
#include "overview.h"
#include "query.h"
#include "surface.h"

#include "com/alg.h"
#include "com/error.h"
#include "com/log.h"
#include "com/print.h"
#include "com/string/vector.h"

#include <algorithm>

namespace
{
bool find_family(const std::vector<VkQueueFamilyProperties>& families, VkQueueFlags flags, VkQueueFlags no_flags, uint32_t* index)
{
        ASSERT((flags & no_flags) == 0);

        for (size_t i = 0; i < families.size(); ++i)
        {
                const VkQueueFamilyProperties& p = families[i];

                if (p.queueCount >= 1 && ((p.queueFlags & flags) == flags) && !(p.queueFlags & no_flags))
                {
                        *index = i;
                        return true;
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

bool device_supports_extensions(VkPhysicalDevice physical_device, const std::vector<std::string>& extensions)
{
        if (extensions.empty())
        {
                return true;
        }

        const std::unordered_set<std::string> extension_set = vulkan::supported_physical_device_extensions(physical_device);

        for (const std::string& e : extensions)
        {
                if (extension_set.count(e) < 1)
                {
                        return false;
                }
        }

        return true;
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
                case vulkan::PhysicalDeviceFeatures::FragmentStoresAndAtomics:
                        if (!device_features.fragmentStoresAndAtomics)
                        {
                                return false;
                        }
                        break;
                case vulkan::PhysicalDeviceFeatures::vertexPipelineStoresAndAtomics:
                        if (!device_features.vertexPipelineStoresAndAtomics)
                        {
                                return false;
                        }
                        break;
                }
        }

        return true;
}
}

namespace vulkan
{
PhysicalDevice::PhysicalDevice(VkPhysicalDevice physical_device, uint32_t graphics, uint32_t graphics_and_compute,
                               uint32_t compute, uint32_t transfer, uint32_t presentation,
                               const VkPhysicalDeviceFeatures& features, const VkPhysicalDeviceProperties& properties,
                               const std::vector<VkQueueFamilyProperties>& families)
        : m_physical_device(physical_device),
          m_graphics_family_index(graphics),
          m_graphics_and_compute_family_index(graphics_and_compute),
          m_compute_family_index(compute),
          m_transfer_family_index(transfer),
          m_presentation_family_index(presentation),
          m_features(features),
          m_properties(properties),
          m_families(families)
{
}
PhysicalDevice::operator VkPhysicalDevice() const noexcept
{
        return m_physical_device;
}
uint32_t PhysicalDevice::graphics() const noexcept
{
        return m_graphics_family_index;
}
uint32_t PhysicalDevice::graphics_and_compute() const noexcept
{
        return m_graphics_and_compute_family_index;
}
uint32_t PhysicalDevice::compute() const noexcept
{
        return m_compute_family_index;
}
uint32_t PhysicalDevice::transfer() const noexcept
{
        return m_transfer_family_index;
}
uint32_t PhysicalDevice::presentation() const noexcept
{
        return m_presentation_family_index;
}
const VkPhysicalDeviceFeatures& PhysicalDevice::features() const noexcept
{
        return m_features;
}
const VkPhysicalDeviceProperties& PhysicalDevice::properties() const noexcept
{
        return m_properties;
}
const std::vector<VkQueueFamilyProperties>& PhysicalDevice::families() const noexcept
{
        return m_families;
}

std::vector<VkPhysicalDevice> physical_devices(VkInstance instance)
{
        uint32_t device_count;
        VkResult result;

        result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumeratePhysicalDevices", result);
        }

        if (device_count < 1)
        {
                error("No Vulkan device found");
        }

        std::vector<VkPhysicalDevice> devices(device_count);

        result = vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumeratePhysicalDevices", result);
        }

        return devices;
}

std::vector<VkQueueFamilyProperties> physical_device_queue_families(VkPhysicalDevice device)
{
        uint32_t queue_family_count;

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        if (queue_family_count < 1)
        {
                return {};
        }

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        return queue_families;
}

std::unordered_set<std::string> supported_physical_device_extensions(VkPhysicalDevice physical_device)
{
        uint32_t extension_count;
        VkResult result;

        result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateDeviceExtensionProperties", result);
        }

        if (extension_count < 1)
        {
                return {};
        }

        std::vector<VkExtensionProperties> extensions(extension_count);

        result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, extensions.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateDeviceExtensionProperties", result);
        }

        std::unordered_set<std::string> extension_set;

        for (const VkExtensionProperties& e : extensions)
        {
                extension_set.emplace(e.extensionName);
        }

        return extension_set;
}

VkPhysicalDeviceFeatures make_enabled_device_features(const std::vector<PhysicalDeviceFeatures>& required_features,
                                                      const std::vector<PhysicalDeviceFeatures>& optional_features,
                                                      const VkPhysicalDeviceFeatures& supported_device_features)
{
        if (there_is_intersection(required_features, optional_features))
        {
                error("Required and optional physical device features intersect");
        }

        VkPhysicalDeviceFeatures device_features = {};

        for (PhysicalDeviceFeatures f : required_features)
        {
                switch (f)
                {
                case PhysicalDeviceFeatures::GeometryShader:
                        if (!supported_device_features.geometryShader)
                        {
                                error("Required physical device feature Geometry Shader is not supported");
                        }
                        device_features.geometryShader = true;
                        break;
                case PhysicalDeviceFeatures::SampleRateShading:
                        if (!supported_device_features.sampleRateShading)
                        {
                                error("Required physical device feature Sample Rate Shading is not supported");
                        }
                        device_features.sampleRateShading = true;
                        break;
                case PhysicalDeviceFeatures::SamplerAnisotropy:
                        if (!supported_device_features.samplerAnisotropy)
                        {
                                error("Required physical device feature Sampler Anisotropy is not supported");
                        }
                        device_features.samplerAnisotropy = true;
                        break;
                case PhysicalDeviceFeatures::TessellationShader:
                        if (!supported_device_features.tessellationShader)
                        {
                                error("Required physical device feature Tessellation Shader is not supported");
                        }
                        device_features.tessellationShader = true;
                        break;
                case PhysicalDeviceFeatures::FragmentStoresAndAtomics:
                        if (!supported_device_features.fragmentStoresAndAtomics)
                        {
                                error("Required physical device feature Fragment Stores And Atomics is not supported");
                        }
                        device_features.fragmentStoresAndAtomics = true;
                        break;
                case PhysicalDeviceFeatures::vertexPipelineStoresAndAtomics:
                        if (!supported_device_features.vertexPipelineStoresAndAtomics)
                        {
                                error("Required physical device feature Vertex Pipeline Stores And Atomics is not supported");
                        }
                        device_features.vertexPipelineStoresAndAtomics = true;
                        break;
                }
        }

        for (PhysicalDeviceFeatures f : optional_features)
        {
                switch (f)
                {
                case PhysicalDeviceFeatures::GeometryShader:
                        device_features.geometryShader = supported_device_features.geometryShader;
                        break;
                case PhysicalDeviceFeatures::SampleRateShading:
                        device_features.sampleRateShading = supported_device_features.sampleRateShading;
                        break;
                case PhysicalDeviceFeatures::SamplerAnisotropy:
                        device_features.samplerAnisotropy = supported_device_features.samplerAnisotropy;
                        break;
                case PhysicalDeviceFeatures::TessellationShader:
                        device_features.tessellationShader = supported_device_features.tessellationShader;
                        break;
                case PhysicalDeviceFeatures::FragmentStoresAndAtomics:
                        device_features.fragmentStoresAndAtomics = supported_device_features.fragmentStoresAndAtomics;
                        break;
                case PhysicalDeviceFeatures::vertexPipelineStoresAndAtomics:
                        device_features.vertexPipelineStoresAndAtomics = supported_device_features.vertexPipelineStoresAndAtomics;
                        break;
                }
        }

        return device_features;
}

PhysicalDevice find_physical_device(VkInstance instance, VkSurfaceKHR surface, int api_version_major, int api_version_minor,
                                    const std::vector<std::string>& required_extensions,
                                    const std::vector<PhysicalDeviceFeatures>& required_features)
{
        LOG(overview_physical_devices(instance, surface));

        const uint32_t required_api_version = VK_MAKE_VERSION(api_version_major, api_version_minor, 0);

        for (const VkPhysicalDevice& physical_device : physical_devices(instance))
        {
                ASSERT(physical_device != VK_NULL_HANDLE);

                VkPhysicalDeviceProperties properties;
                VkPhysicalDeviceFeatures features;

                vkGetPhysicalDeviceProperties(physical_device, &properties);
                vkGetPhysicalDeviceFeatures(physical_device, &features);

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

                if (!device_supports_extensions(physical_device, required_extensions))
                {
                        continue;
                }

                if (!surface_suitable(surface, physical_device))
                {
                        continue;
                }

                const std::vector<VkQueueFamilyProperties> families = physical_device_queue_families(physical_device);

                uint32_t graphics;
                uint32_t graphics_and_compute;
                uint32_t compute;
                uint32_t transfer;
                uint32_t presentation;

                if (!find_family(families, VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, &graphics))
                {
                        if (!find_family(families, VK_QUEUE_GRAPHICS_BIT, 0, &graphics))
                        {
                                continue;
                        }
                }

                if (!find_family(families, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0, &graphics_and_compute))
                {
                        continue;
                }

                if (!find_family(families, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_GRAPHICS_BIT, &compute))
                {
                        if (!find_family(families, VK_QUEUE_COMPUTE_BIT, 0, &compute))
                        {
                                continue;
                        }
                }

                if (!find_family(families, VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, &transfer))
                {
                        // Наличие VK_QUEUE_GRAPHICS_BIT или VK_QUEUE_COMPUTE_BIT означает VK_QUEUE_TRANSFER_BIT
                        if (!find_family(families, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0, &transfer))
                        {
                                continue;
                        }
                }

                if (!find_presentation_family(surface, physical_device, families, &presentation))
                {
                        continue;
                }

                std::string s;
                s += "Graphics: family index = " + to_string(graphics) +
                     ", queue count = " + to_string(families[graphics].queueCount) + "\n";
                s += "Graphics and compute: family index = " + to_string(graphics_and_compute) +
                     ", queue count = " + to_string(families[graphics_and_compute].queueCount) + "\n";
                s += "Compute: family index = " + to_string(compute) +
                     ", queue count = " + to_string(families[compute].queueCount) + "\n";
                s += "Transfer: family index = " + to_string(transfer) +
                     ", queue count = " + to_string(families[transfer].queueCount) + "\n";
                s += "Presentation: family index = " + to_string(presentation) +
                     ", queue count = " + to_string(families[presentation].queueCount);
                LOG(s);

                return PhysicalDevice(physical_device, graphics, graphics_and_compute, compute, transfer, presentation, features,
                                      properties, families);
        }

        error("Failed to find a suitable Vulkan physical device");
}

Device create_device(VkPhysicalDevice physical_device, const std::vector<uint32_t>& family_indices,
                     const std::vector<std::string>& required_extensions,
                     const std::vector<std::string>& required_validation_layers, const VkPhysicalDeviceFeatures& enabled_features)
{
        if (family_indices.empty())
        {
                error("No family indices for device creation");
        }

        constexpr float queue_priority = 1;
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        for (uint32_t unique_queue_family_index : std::unordered_set<uint32_t>(family_indices.cbegin(), family_indices.cend()))
        {
                VkDeviceQueueCreateInfo queue_create_info = {};
                queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_create_info.queueFamilyIndex = unique_queue_family_index;
                queue_create_info.queueCount = 1;
                queue_create_info.pQueuePriorities = &queue_priority;

                queue_create_infos.push_back(queue_create_info);
        }

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pEnabledFeatures = &enabled_features;

        const std::vector<const char*> extensions = const_char_pointer_vector(required_extensions);
        if (extensions.size() > 0)
        {
                create_info.enabledExtensionCount = extensions.size();
                create_info.ppEnabledExtensionNames = extensions.data();
        }

        const std::vector<const char*> validation_layers = const_char_pointer_vector(required_validation_layers);
        if (validation_layers.size() > 0)
        {
                create_info.enabledLayerCount = validation_layers.size();
                create_info.ppEnabledLayerNames = validation_layers.data();
        }

        return Device(physical_device, create_info);
}
}
