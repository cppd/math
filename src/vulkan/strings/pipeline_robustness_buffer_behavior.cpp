/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "pipeline_robustness_buffer_behavior.h"

#include <src/com/enum.h>
#include <src/com/print.h>

#include <vulkan/vulkan_core.h>

#include <string>

#define CASE(parameter) \
        case parameter: \
                return #parameter;

namespace ns::vulkan::strings
{
std::string pipeline_robustness_buffer_behavior_to_string(
        const VkPipelineRobustnessBufferBehavior pipeline_robustness_buffer_behavior)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (pipeline_robustness_buffer_behavior)
        {
                CASE(VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT)
                CASE(VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED)
                CASE(VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS)
                CASE(VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2)
        }
#pragma GCC diagnostic pop

        return "Unknown VkPipelineRobustnessBufferBehavior "
               + to_string(enum_to_int(pipeline_robustness_buffer_behavior));
}
}
