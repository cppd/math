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

#include "shader.h"

#include <src/com/error.h>

#include <cstdint>
#include <span>
#include <vulkan/vulkan_core.h>

namespace ns::vulkan
{
namespace
{
constexpr const char* ENTRY_POINT_NAME = "main";
}

Shader::Shader(const VkDevice device, const std::span<const std::uint32_t>& code, const VkShaderStageFlagBits stage)
        : module_(device, code),
          stage_(stage)
{
        ASSERT(stage == VK_SHADER_STAGE_VERTEX_BIT || stage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
               || stage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT || stage == VK_SHADER_STAGE_GEOMETRY_BIT
               || stage == VK_SHADER_STAGE_FRAGMENT_BIT || stage == VK_SHADER_STAGE_COMPUTE_BIT
               || stage == VK_SHADER_STAGE_RAYGEN_BIT_KHR || stage == VK_SHADER_STAGE_ANY_HIT_BIT_KHR
               || stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR || stage == VK_SHADER_STAGE_MISS_BIT_KHR
               || stage == VK_SHADER_STAGE_INTERSECTION_BIT_KHR || stage == VK_SHADER_STAGE_CALLABLE_BIT_KHR);
}

VkShaderModule Shader::module() const
{
        return module_;
}

VkShaderStageFlagBits Shader::stage() const
{
        return stage_;
}

const char* Shader::entry_point_name()
{
        return ENTRY_POINT_NAME;
}
}
