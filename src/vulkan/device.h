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

#pragma once

#include "functionality.h"
#include "objects.h"
#include "physical_device.h"
#include "physical_device_info.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ns::vulkan
{
class Device final
{
        const PhysicalDevice* physical_device_ = nullptr;
        PhysicalDeviceFeatures features_;
        std::unordered_set<std::string> extensions_;
        handle::Device device_;
        std::unordered_map<std::uint32_t, std::vector<VkQueue>> queues_;

public:
        Device(const PhysicalDevice* physical_device,
               const std::unordered_map<std::uint32_t, std::uint32_t>& queue_families,
               const DeviceFunctionality& functionality);

        operator VkDevice() const& noexcept
        {
                return device_;
        }
        operator VkDevice() const&& = delete;

        VkPhysicalDevice physical_device() const;

        const PhysicalDeviceProperties& properties() const;
        const std::unordered_set<std::string>& extensions() const;
        const PhysicalDeviceFeatures& features() const;

        Queue queue(std::uint32_t family_index, std::uint32_t queue_index) const;
};
}
