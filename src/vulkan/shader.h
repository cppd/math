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

#pragma once

#include "objects.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <span>

namespace ns::vulkan
{
class Shader final
{
        handle::ShaderModule module_;
        VkShaderStageFlagBits stage_;

public:
        Shader(VkDevice device, const std::span<const std::uint32_t>& code, VkShaderStageFlagBits stage);

        [[nodiscard]] VkShaderModule module() const;
        [[nodiscard]] VkShaderStageFlagBits stage() const;

        [[nodiscard]] static const char* entry_point_name();
};
}
