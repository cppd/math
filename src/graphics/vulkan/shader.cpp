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

#if defined(VULKAN_FOUND)

#include "shader.h"

#include "com/error.h"

namespace vulkan
{
Shader::Shader(VkDevice device, const Span<const uint32_t>& code, VkShaderStageFlagBits type,
               const std::string_view& entry_point_name)
        : m_module(device, code), m_stage(type), m_entry_point_name(entry_point_name)
{
        ASSERT(type == VK_SHADER_STAGE_VERTEX_BIT || type == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT ||
               type == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT || type == VK_SHADER_STAGE_GEOMETRY_BIT ||
               type == VK_SHADER_STAGE_FRAGMENT_BIT || type == VK_SHADER_STAGE_COMPUTE_BIT);
        ASSERT(entry_point_name.size() > 0);
}

Shader::~Shader() = default;

VkShaderModule Shader::module() const noexcept
{
        return m_module;
}

VkShaderStageFlagBits Shader::stage() const noexcept
{
        return m_stage;
}

const char* Shader::entry_point_name() const noexcept
{
        return m_entry_point_name.c_str();
}

//

VertexShader::VertexShader(VkDevice device, const Span<const uint32_t>& code, const std::string_view& entry_point_name)
        : Shader(device, code, VK_SHADER_STAGE_VERTEX_BIT, entry_point_name)
{
}

TesselationControlShader::TesselationControlShader(VkDevice device, const Span<const uint32_t>& code,
                                                   const std::string_view& entry_point_name)
        : Shader(device, code, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, entry_point_name)
{
}

TesselationEvaluationShader ::TesselationEvaluationShader(VkDevice device, const Span<const uint32_t>& code,
                                                          const std::string_view& entry_point_name)
        : Shader(device, code, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, entry_point_name)
{
}

GeometryShader::GeometryShader(VkDevice device, const Span<const uint32_t>& code, const std::string_view& entry_point_name)
        : Shader(device, code, VK_SHADER_STAGE_GEOMETRY_BIT, entry_point_name)
{
}

FragmentShader::FragmentShader(VkDevice device, const Span<const uint32_t>& code, const std::string_view& entry_point_name)
        : Shader(device, code, VK_SHADER_STAGE_FRAGMENT_BIT, entry_point_name)
{
}

ComputeShader::ComputeShader(VkDevice device, const Span<const uint32_t>& code, const std::string_view& entry_point_name)
        : Shader(device, code, VK_SHADER_STAGE_COMPUTE_BIT, entry_point_name)
{
}
}

#endif
