/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "shader.h"

#include <memory>
#include <vector>

namespace ns::vulkan
{
class PipelineShaderStageCreateInfo final
{
        std::vector<VkPipelineShaderStageCreateInfo> create_info_;
        std::vector<std::unique_ptr<VkSpecializationInfo>> specialization_info_;

        void init_create_info(const std::vector<const Shader*>& shaders);

        void init_specialization_info(
                const std::vector<const Shader*>& shaders,
                const std::vector<VkSpecializationInfo>& constants);

public:
        PipelineShaderStageCreateInfo(
                const std::vector<const Shader*>& shaders,
                const std::vector<VkSpecializationInfo>* constants);

        PipelineShaderStageCreateInfo(
                const std::vector<const Shader*>& shaders,
                const std::vector<VkSpecializationInfo>& constants);

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
