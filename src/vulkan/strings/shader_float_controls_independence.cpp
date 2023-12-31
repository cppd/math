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

#include "shader_float_controls_independence.h"

#include <src/com/enum.h>
#include <src/com/print.h>

#include <vulkan/vulkan_core.h>

#include <string>

#define CASE(parameter) \
        case parameter: \
                return #parameter;

namespace ns::vulkan
{
std::string shader_float_controls_independence_to_string(
        const VkShaderFloatControlsIndependence shader_float_controls_independence)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
        switch (shader_float_controls_independence)
        {
                CASE(VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_32_BIT_ONLY)
                CASE(VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL)
                CASE(VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_NONE)
        }
#pragma GCC diagnostic pop

        return "Unknown VkShaderFloatControlsIndependence "
               + to_string(enum_to_int(shader_float_controls_independence));
}
}
