/*
Copyright (C) 2017-2020 Topological Manifold

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

namespace ns::vulkan
{
Shader::Shader(
        VkDevice device,
        const std::span<const uint32_t>& code,
        VkShaderStageFlagBits stage,
        std::string entry_point_name)
        : m_module(device, code), m_stage(stage), m_entry_point_name(std::move(entry_point_name))
{
        ASSERT(stage == VK_SHADER_STAGE_VERTEX_BIT || stage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT
               || stage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT || stage == VK_SHADER_STAGE_GEOMETRY_BIT
               || stage == VK_SHADER_STAGE_FRAGMENT_BIT || stage == VK_SHADER_STAGE_COMPUTE_BIT);
        ASSERT(!m_entry_point_name.empty());
}

VkShaderModule Shader::module() const
{
        return m_module;
}

VkShaderStageFlagBits Shader::stage() const
{
        return m_stage;
}

const char* Shader::entry_point_name() const
{
        return m_entry_point_name.c_str();
}

//

VertexShader::VertexShader(VkDevice device, const std::span<const uint32_t>& code, std::string entry_point_name)
        : Shader(device, code, VK_SHADER_STAGE_VERTEX_BIT, std::move(entry_point_name))
{
}

TesselationControlShader::TesselationControlShader(
        VkDevice device,
        const std::span<const uint32_t>& code,
        std::string entry_point_name)
        : Shader(device, code, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, std::move(entry_point_name))
{
}

TesselationEvaluationShader ::TesselationEvaluationShader(
        VkDevice device,
        const std::span<const uint32_t>& code,
        std::string entry_point_name)
        : Shader(device, code, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, std::move(entry_point_name))
{
}

GeometryShader::GeometryShader(VkDevice device, const std::span<const uint32_t>& code, std::string entry_point_name)
        : Shader(device, code, VK_SHADER_STAGE_GEOMETRY_BIT, std::move(entry_point_name))
{
}

FragmentShader::FragmentShader(VkDevice device, const std::span<const uint32_t>& code, std::string entry_point_name)
        : Shader(device, code, VK_SHADER_STAGE_FRAGMENT_BIT, std::move(entry_point_name))
{
}

ComputeShader::ComputeShader(VkDevice device, const std::span<const uint32_t>& code, std::string entry_point_name)
        : Shader(device, code, VK_SHADER_STAGE_COMPUTE_BIT, std::move(entry_point_name))
{
}
}
