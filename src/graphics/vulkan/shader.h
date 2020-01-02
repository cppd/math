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

#pragma once

#include "objects.h"

#include <string>
#include <string_view>

namespace vulkan
{
class Shader
{
        ShaderModule m_module;
        VkShaderStageFlagBits m_stage;
        std::string m_entry_point_name;

protected:
        Shader(VkDevice device, const Span<const uint32_t>& code, VkShaderStageFlagBits stage,
               const std::string_view& entry_point_name);

public:
        VkShaderModule module() const;
        VkShaderStageFlagBits stage() const;
        const char* entry_point_name() const;
};

class VertexShader final : public Shader
{
public:
        VertexShader(VkDevice device, const Span<const uint32_t>& code, const std::string_view& entry_point_name);
};

class TesselationControlShader final : public Shader
{
public:
        TesselationControlShader(VkDevice device, const Span<const uint32_t>& code, const std::string_view& entry_point_name);
};

class TesselationEvaluationShader final : public Shader
{
public:
        TesselationEvaluationShader(VkDevice device, const Span<const uint32_t>& code, const std::string_view& entry_point_name);
};

class GeometryShader final : public Shader
{
public:
        GeometryShader(VkDevice device, const Span<const uint32_t>& code, const std::string_view& entry_point_name);
};

class FragmentShader final : public Shader
{
public:
        FragmentShader(VkDevice device, const Span<const uint32_t>& code, const std::string_view& entry_point_name);
};

class ComputeShader final : public Shader
{
public:
        ComputeShader(VkDevice device, const Span<const uint32_t>& code, const std::string_view& entry_point_name);
};
}
