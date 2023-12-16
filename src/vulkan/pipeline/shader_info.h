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

#pragma once

#include "../shader.h"

#include <cstdint>
#include <vector>

namespace ns::vulkan
{
class PipelineShaderStageCreateInfo final
{
        std::vector<VkPipelineShaderStageCreateInfo> create_info_;
        std::vector<VkSpecializationInfo> specialization_info_;

public:
        PipelineShaderStageCreateInfo(
                const std::vector<const Shader*>& shaders,
                std::vector<VkSpecializationInfo> specialization_info);

        PipelineShaderStageCreateInfo(const Shader* shader, const VkSpecializationInfo* specialization_info);

        PipelineShaderStageCreateInfo(const PipelineShaderStageCreateInfo&) = delete;
        PipelineShaderStageCreateInfo& operator=(const PipelineShaderStageCreateInfo&) = delete;
        PipelineShaderStageCreateInfo(PipelineShaderStageCreateInfo&&) = delete;
        PipelineShaderStageCreateInfo& operator=(PipelineShaderStageCreateInfo&&) = delete;

        [[nodiscard]] std::uint32_t size() const
        {
                return create_info_.size();
        }

        [[nodiscard]] const VkPipelineShaderStageCreateInfo* data() const
        {
                return create_info_.data();
        }
};
}
