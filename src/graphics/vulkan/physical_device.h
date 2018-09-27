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

#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan
{
enum class PhysicalDeviceFeatures
{
        GeometryShader,
        SampleRateShading,
        SamplerAnisotropy,
        TessellationShader
};

struct SwapChainDetails
{
        VkSurfaceCapabilitiesKHR surface_capabilities;
        std::vector<VkSurfaceFormatKHR> surface_formats;
        std::vector<VkPresentModeKHR> present_modes;
};

class PhysicalDevice
{
        VkPhysicalDevice m_device;

        // family_indices
        uint32_t m_graphics, m_compute, m_transfer, m_presentation;

        VkPhysicalDeviceFeatures m_features;

public:
        PhysicalDevice(VkPhysicalDevice device, uint32_t graphics, uint32_t compute, uint32_t transfer, uint32_t presentation,
                       const VkPhysicalDeviceFeatures& features)
                : m_device(device),
                  m_graphics(graphics),
                  m_compute(compute),
                  m_transfer(transfer),
                  m_presentation(presentation),
                  m_features(features)
        {
        }

        operator VkPhysicalDevice() const noexcept
        {
                return m_device;
        }
        uint32_t graphics() const noexcept
        {
                return m_graphics;
        }
        uint32_t compute() const noexcept
        {
                return m_compute;
        }
        uint32_t transfer() const noexcept
        {
                return m_transfer;
        }
        uint32_t presentation() const noexcept
        {
                return m_presentation;
        }
        const VkPhysicalDeviceFeatures& features() const noexcept
        {
                return m_features;
        }
};

VkPhysicalDeviceFeatures make_enabled_device_features(const std::vector<PhysicalDeviceFeatures>& required_features,
                                                      const std::vector<PhysicalDeviceFeatures>& optional_features,
                                                      const VkPhysicalDeviceFeatures& supported_device_features);

bool find_swap_chain_details(VkSurfaceKHR surface, VkPhysicalDevice device, SwapChainDetails* swap_chain_details);

PhysicalDevice find_physical_device(VkInstance instance, VkSurfaceKHR surface, int api_version_major, int api_version_minor,
                                    const std::vector<std::string>& required_extensions,
                                    const std::vector<PhysicalDeviceFeatures>& required_features);
}
