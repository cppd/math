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

#include "physical_device_type.h"

#include <src/com/enum.h>
#include <src/com/print.h>

namespace ns::vulkan
{
std::string physical_device_type_to_string(const VkPhysicalDeviceType type)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (type)
        {
        case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                return "Unknown";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                return "Integrated GPU";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                return "Discrete GPU";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                return "Virtual GPU";
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
                return "CPU";
        }
#pragma GCC diagnostic pop

        return "Unknown VkPhysicalDeviceType " + to_string(enum_to_int(type));
}
}
