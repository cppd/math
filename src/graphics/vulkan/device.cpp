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
        ASSERT(flags != 0);
        ASSERT((flags & no_flags) == 0);

        for (size_t i = 0; i < families.size(); ++i)
        {
                const VkQueueFamilyProperties& p = families[i];

                if (p.queueCount < 1)
                {
                        continue;
                }

                if (((p.queueFlags & flags) == flags) && !(p.queueFlags & no_flags))
                {
                        *index = i;
                        return true;
                }
        }
        return false;
}

std::vector<bool> find_presentation_support(VkSurfaceKHR surface, VkPhysicalDevice device,
                                            const std::vector<VkQueueFamilyProperties>& queue_families)
{
        if (surface == VK_NULL_HANDLE)
        {
                return std::vector<bool>(queue_families.size(), false);
        }

        std::vector<bool> presentation_supported(queue_families.size());

        for (uint32_t i = 0; i < queue_families.size(); ++i)
        {
                if (queue_families[i].queueCount < 1)
                {
                        continue;
                }

                VkBool32 supported;

                VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supported);
                if (result != VK_SUCCESS)
                {
                        vulkan::vulkan_function_error("vkGetPhysicalDeviceSurfaceSupportKHR", result);
                }

                presentation_supported[i] = (supported == VK_TRUE);
        }

        return presentation_supported;
}

bool features_are_supported(const std::vector<vulkan::PhysicalDeviceFeatures>& required_features,
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
                case vulkan::PhysicalDeviceFeatures::VertexPipelineStoresAndAtomics:
                        if (!device_features.vertexPipelineStoresAndAtomics)
                        {
                                return false;
                        }
                        break;
                }
        }

        return true;
}

std::vector<VkQueueFamilyProperties> find_queue_families(VkPhysicalDevice device)
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

std::unordered_set<std::string> find_extensions(VkPhysicalDevice device)
{
        uint32_t extension_count;
        VkResult result;

        result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkEnumerateDeviceExtensionProperties", result);
        }

        if (extension_count < 1)
        {
                return {};
        }

        std::vector<VkExtensionProperties> extensions(extension_count);

        result = vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, extensions.data());
        if (result != VK_SUCCESS)
        {
                vulkan::vulkan_function_error("vkEnumerateDeviceExtensionProperties", result);
        }

        std::unordered_set<std::string> extension_set;

        for (const VkExtensionProperties& e : extensions)
        {
                extension_set.emplace(e.extensionName);
        }

        return extension_set;
}

VkPhysicalDeviceFeatures make_enabled_device_features(const std::vector<vulkan::PhysicalDeviceFeatures>& required_features,
                                                      const std::vector<vulkan::PhysicalDeviceFeatures>& optional_features,
                                                      const VkPhysicalDeviceFeatures& supported_device_features)
{
        if (there_is_intersection(required_features, optional_features))
        {
                error("Required and optional physical device features intersect");
        }

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
                case vulkan::PhysicalDeviceFeatures::FragmentStoresAndAtomics:
                        if (!supported_device_features.fragmentStoresAndAtomics)
                        {
                                error("Required physical device feature Fragment Stores And Atomics is not supported");
                        }
                        device_features.fragmentStoresAndAtomics = true;
                        break;
                case vulkan::PhysicalDeviceFeatures::VertexPipelineStoresAndAtomics:
                        if (!supported_device_features.vertexPipelineStoresAndAtomics)
                        {
                                error("Required physical device feature Vertex Pipeline Stores And Atomics is not supported");
                        }
                        device_features.vertexPipelineStoresAndAtomics = true;
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
                case vulkan::PhysicalDeviceFeatures::FragmentStoresAndAtomics:
                        device_features.fragmentStoresAndAtomics = supported_device_features.fragmentStoresAndAtomics;
                        break;
                case vulkan::PhysicalDeviceFeatures::VertexPipelineStoresAndAtomics:
                        device_features.vertexPipelineStoresAndAtomics = supported_device_features.vertexPipelineStoresAndAtomics;
                        break;
                }
        }

        return device_features;
}
}

namespace vulkan
{
PhysicalDevice::PhysicalDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface) : m_physical_device(physical_device)
{
        ASSERT(physical_device != VK_NULL_HANDLE);

        vkGetPhysicalDeviceProperties(m_physical_device, &m_properties);
        vkGetPhysicalDeviceFeatures(m_physical_device, &m_features);

        m_queue_families = find_queue_families(physical_device);
        m_presentation_supported = find_presentation_support(surface, m_physical_device, m_queue_families);
        m_supported_extensions = find_extensions(m_physical_device);

        ASSERT(m_queue_families.size() == m_presentation_supported.size());
}

PhysicalDevice::operator VkPhysicalDevice() const
{
        return m_physical_device;
}

const VkPhysicalDeviceFeatures& PhysicalDevice::features() const
{
        return m_features;
}

const VkPhysicalDeviceProperties& PhysicalDevice::properties() const
{
        return m_properties;
}

const std::vector<VkQueueFamilyProperties>& PhysicalDevice::queue_families() const
{
        return m_queue_families;
}

const std::unordered_set<std::string>& PhysicalDevice::supported_extensions() const
{
        return m_supported_extensions;
}

uint32_t PhysicalDevice::family_index(VkQueueFlags set_flags, VkQueueFlags not_set_flags, VkQueueFlags default_flags) const
{
        uint32_t index;
        if (set_flags && find_family(m_queue_families, set_flags, not_set_flags, &index))
        {
                return index;
        }
        if (default_flags && find_family(m_queue_families, default_flags, 0, &index))
        {
                return index;
        }
        error("Queue family not found, flags " + to_string(set_flags) + " " + to_string(not_set_flags) + " " +
              to_string(default_flags));
}

uint32_t PhysicalDevice::presentation_family_index() const
{
        for (size_t i = 0; i < m_presentation_supported.size(); ++i)
        {
                if (m_presentation_supported[i])
                {
                        return i;
                }
        }
        error("Presentation family not found");
}

bool PhysicalDevice::supports_extensions(const std::vector<std::string>& extensions) const
{
        return std::all_of(extensions.cbegin(), extensions.cend(),
                           [&](const std::string& e) { return m_supported_extensions.count(e) >= 1; });
}

bool PhysicalDevice::queue_family_supports_presentation(uint32_t index) const
{
        ASSERT(index < m_presentation_supported.size());

        return m_presentation_supported[index];
}

Device PhysicalDevice::create_device(const std::unordered_map<uint32_t, uint32_t>& queue_families,
                                     const std::vector<std::string>& required_extensions,
                                     const std::vector<PhysicalDeviceFeatures>& required_features,
                                     const std::vector<PhysicalDeviceFeatures>& optional_features) const
{
        ASSERT(std::all_of(queue_families.cbegin(), queue_families.cend(),
                           [&](const auto& v) { return v.first < m_queue_families.size(); }));
        ASSERT(std::all_of(queue_families.cbegin(), queue_families.cend(), [](const auto& v) { return v.second > 0; }));
        ASSERT(std::all_of(queue_families.cbegin(), queue_families.cend(),
                           [&](const auto& v) { return v.second <= m_queue_families[v.first].queueCount; }));

        if (queue_families.empty())
        {
                error("No queue families for device creation");
        }

        std::vector<std::vector<float>> queue_priorities(queue_families.size());
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos(queue_families.size());
        unsigned i = 0;
        for (const auto& [queue_family_index, queue_count] : queue_families)
        {
                queue_priorities[i].resize(queue_count, 1);

                VkDeviceQueueCreateInfo queue_create_info = {};
                queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_create_info.queueFamilyIndex = queue_family_index;
                queue_create_info.queueCount = queue_count;
                queue_create_info.pQueuePriorities = queue_priorities[i].data();
                queue_create_infos[i] = queue_create_info;

                ++i;
        }

        const VkPhysicalDeviceFeatures enabled_features =
                make_enabled_device_features(required_features, optional_features, m_features);

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

        return Device(m_physical_device, create_info);
}

//

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

PhysicalDevice find_physical_device(VkInstance instance, VkSurfaceKHR surface, int api_version_major, int api_version_minor,
                                    const std::vector<std::string>& required_extensions,
                                    const std::vector<PhysicalDeviceFeatures>& required_features)
{
        LOG(overview_physical_devices(instance, surface));

        const uint32_t required_api_version = VK_MAKE_VERSION(api_version_major, api_version_minor, 0);

        for (const VkPhysicalDevice& d : physical_devices(instance))
        {
                PhysicalDevice physical_device(d, surface);

                if (physical_device.properties().deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                    physical_device.properties().deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &&
                    physical_device.properties().deviceType != VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU &&
                    physical_device.properties().deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU)
                {
                        continue;
                }

                if (required_api_version > physical_device.properties().apiVersion)
                {
                        continue;
                }

                if (!features_are_supported(required_features, physical_device.features()))
                {
                        continue;
                }

                if (!physical_device.supports_extensions(required_extensions))
                {
                        continue;
                }

                try
                {
                        physical_device.family_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0, 0);
                }
                catch (...)
                {
                        continue;
                }

                if (surface != VK_NULL_HANDLE)
                {
                        try
                        {
                                physical_device.presentation_family_index();
                        }
                        catch (...)
                        {
                                continue;
                        }

                        if (!surface_suitable(surface, physical_device))
                        {
                                continue;
                        }
                }

                return physical_device;
        }

        error("Failed to find a suitable Vulkan physical device");
}
}
