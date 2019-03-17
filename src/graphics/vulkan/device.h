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

#pragma once

#include <string>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan
{
enum class PhysicalDeviceFeatures
{
        GeometryShader,
        SampleRateShading,
        SamplerAnisotropy,
        TessellationShader,
        FragmentStoresAndAtomics,
        vertexPipelineStoresAndAtomics
};

class PhysicalDevice final
{
        VkPhysicalDevice m_physical_device;
        VkPhysicalDeviceFeatures m_features;
        VkPhysicalDeviceProperties m_properties;
        std::vector<VkQueueFamilyProperties> m_families;
        std::vector<bool> m_presentation_supported;

public:
        PhysicalDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

        operator VkPhysicalDevice() const noexcept;

        const VkPhysicalDeviceFeatures& features() const noexcept;
        const VkPhysicalDeviceProperties& properties() const noexcept;
        const std::vector<VkQueueFamilyProperties>& families() const noexcept;

        uint32_t family_index(VkQueueFlags set_flags, VkQueueFlags not_set_flags, VkQueueFlags default_flags) const;
        uint32_t presentation_family_index() const;
};

std::vector<VkPhysicalDevice> physical_devices(VkInstance instance);
std::vector<VkQueueFamilyProperties> physical_device_queue_families(VkPhysicalDevice device);
std::unordered_set<std::string> supported_physical_device_extensions(VkPhysicalDevice physical_device);

VkPhysicalDeviceFeatures make_enabled_device_features(const std::vector<PhysicalDeviceFeatures>& required_features,
                                                      const std::vector<PhysicalDeviceFeatures>& optional_features,
                                                      const VkPhysicalDeviceFeatures& supported_device_features);

PhysicalDevice find_physical_device(VkInstance instance, VkSurfaceKHR surface, int api_version_major, int api_version_minor,
                                    const std::vector<std::string>& required_extensions,
                                    const std::vector<PhysicalDeviceFeatures>& required_features);
}
