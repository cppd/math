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

#include "shader_info.h"

#include "../shader.h"

#include <src/com/error.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <utility>
#include <vector>

namespace ns::vulkan
{
namespace
{
std::vector<VkPipelineShaderStageCreateInfo> create_info(const std::vector<const Shader*>& shaders)
{
        std::vector<VkPipelineShaderStageCreateInfo> res;
        res.reserve(shaders.size());

        for (const Shader* const shader : shaders)
        {
                ASSERT(shader);
                auto& v = res.emplace_back();
                v.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                v.stage = shader->stage();
                v.module = shader->module();
                v.pName = shader->entry_point_name();
        }

        return res;
}

void set_info_pointers(
        std::vector<VkPipelineShaderStageCreateInfo>* const create_info,
        const std::vector<VkSpecializationInfo>* const specialization_info)
{
        ASSERT(create_info->size() == specialization_info->size());
        for (std::size_t i = 0; i < specialization_info->size(); ++i)
        {
                const VkSpecializationInfo& info = (*specialization_info)[i];
                if (info.mapEntryCount > 0)
                {
                        (*create_info)[i].pSpecializationInfo = &info;
                }
        }
}
}

PipelineShaderStageCreateInfo::PipelineShaderStageCreateInfo(
        const std::vector<const Shader*>& shaders,
        std::vector<VkSpecializationInfo> specialization_info)
        : create_info_(create_info(shaders))
{
        if (!specialization_info.empty())
        {
                ASSERT(shaders.size() == specialization_info.size());
                specialization_info_ = std::move(specialization_info);
                set_info_pointers(&create_info_, &specialization_info_);
        }
}

PipelineShaderStageCreateInfo::PipelineShaderStageCreateInfo(
        const Shader* const shader,
        const VkSpecializationInfo* const specialization_info)
        : create_info_(create_info({shader}))
{
        if (specialization_info)
        {
                specialization_info_.push_back(*specialization_info);
                set_info_pointers(&create_info_, &specialization_info_);
        }
}
}
