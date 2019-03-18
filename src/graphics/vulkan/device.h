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

#include "objects.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vulkan
{
enum class PhysicalDeviceFeatures
{
        GeometryShader,
        SampleRateShading,
        SamplerAnisotropy,
        TessellationShader,
        FragmentStoresAndAtomics,
        VertexPipelineStoresAndAtomics
};

class PhysicalDevice final
{
        VkPhysicalDevice m_physical_device;
        VkPhysicalDeviceFeatures m_features;
        VkPhysicalDeviceProperties m_properties;
        std::vector<VkQueueFamilyProperties> m_queue_families;
        std::vector<bool> m_presentation_supported;
        std::unordered_set<std::string> m_supported_extensions;

public:
        PhysicalDevice(VkPhysicalDevice physical_device, VkSurfaceKHR surface);

        operator VkPhysicalDevice() const noexcept;

        const VkPhysicalDeviceFeatures& features() const noexcept;
        const VkPhysicalDeviceProperties& properties() const noexcept;
        const std::vector<VkQueueFamilyProperties>& queue_families() const noexcept;
        const std::unordered_set<std::string>& supported_extensions() const noexcept;

        uint32_t family_index(VkQueueFlags set_flags, VkQueueFlags not_set_flags, VkQueueFlags default_flags) const;
        uint32_t presentation_family_index() const;
        bool supports_extensions(const std::vector<std::string>& extensions) const;
        bool queue_family_supports_presentation(uint32_t index) const;

        Device create_device(const std::unordered_map<uint32_t, uint32_t>& queue_families,
                             const std::vector<std::string>& required_extensions,
                             const std::vector<std::string>& required_validation_layers,
                             const std::vector<PhysicalDeviceFeatures>& required_features,
                             const std::vector<PhysicalDeviceFeatures>& optional_features) const;
};

std::vector<VkPhysicalDevice> physical_devices(VkInstance instance);

PhysicalDevice find_physical_device(VkInstance instance, VkSurfaceKHR surface, int api_version_major, int api_version_minor,
                                    const std::vector<std::string>& required_extensions,
                                    const std::vector<PhysicalDeviceFeatures>& required_features);
}
