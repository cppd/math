/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "physical_device_type.h"

#include <src/com/enum.h>
#include <src/com/print.h>

#include <vulkan/vulkan_core.h>

#include <string>

#define CASE(parameter) \
        case parameter: \
                return #parameter;

namespace ns::vulkan
{
std::string physical_device_type_to_string(const VkPhysicalDeviceType physical_device_type)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (physical_device_type)
        {
                CASE(VK_PHYSICAL_DEVICE_TYPE_OTHER)
                CASE(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                CASE(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                CASE(VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
                CASE(VK_PHYSICAL_DEVICE_TYPE_CPU)
        }
#pragma GCC diagnostic pop

        return "Unknown VkPhysicalDeviceType " + to_string(enum_to_int(physical_device_type));
}
}
