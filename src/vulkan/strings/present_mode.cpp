/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "present_mode.h"

#include <src/com/enum.h>
#include <src/com/print.h>

#include <vulkan/vulkan_core.h>

#include <string>

#define CASE(parameter) \
        case parameter: \
                return #parameter;

namespace ns::vulkan
{
std::string present_mode_to_string(const VkPresentModeKHR present_mode)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (present_mode)
        {
                CASE(VK_PRESENT_MODE_IMMEDIATE_KHR)
                CASE(VK_PRESENT_MODE_MAILBOX_KHR)
                CASE(VK_PRESENT_MODE_FIFO_KHR)
                CASE(VK_PRESENT_MODE_FIFO_RELAXED_KHR)
                CASE(VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR)
                CASE(VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR)
        }
#pragma GCC diagnostic pop

        return "Unknown VkPresentModeKHR " + to_string(enum_to_int(present_mode));
}
}
