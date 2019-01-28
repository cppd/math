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

#include <optional>
#include <string>
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
        vertexPipelineStoresAndAtomics
};

class PhysicalDevice
{
        VkPhysicalDevice m_physical_device;
        // family_indices
        uint32_t m_graphics, m_compute, m_transfer, m_presentation;
        VkPhysicalDeviceFeatures m_features;
        VkPhysicalDeviceProperties m_properties;

public:
        PhysicalDevice(VkPhysicalDevice physical_device, uint32_t graphics, uint32_t compute, uint32_t transfer,
                       uint32_t presentation, const VkPhysicalDeviceFeatures& features,
                       const VkPhysicalDeviceProperties& properties);

        operator VkPhysicalDevice() const noexcept;

        uint32_t graphics() const noexcept;
        uint32_t compute() const noexcept;
        uint32_t transfer() const noexcept;
        uint32_t presentation() const noexcept;

        const VkPhysicalDeviceFeatures& features() const noexcept;
        const VkPhysicalDeviceProperties& properties() const noexcept;
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

Device create_device(VkPhysicalDevice physical_device, const std::vector<uint32_t>& family_indices,
                     const std::vector<std::string>& required_extensions,
                     const std::vector<std::string>& required_validation_layers,
                     const VkPhysicalDeviceFeatures& enabled_features);

VkQueue device_queue(VkDevice device, uint32_t queue_family_index, uint32_t queue_index);
}
