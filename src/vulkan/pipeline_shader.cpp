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

#include "pipeline_shader.h"

#include <src/com/error.h>

namespace ns::vulkan
{
void PipelineShaderStageCreateInfo::init_create_info(const std::vector<const Shader*>& shaders)
{
        create_info_.resize(shaders.size());

        for (std::size_t i = 0; i < shaders.size(); ++i)
        {
                const Shader* const shader = shaders[i];
                ASSERT(shader);

                create_info_[i] = {};
                create_info_[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                create_info_[i].stage = shader->stage();
                create_info_[i].module = shader->module();
                create_info_[i].pName = shader->entry_point_name();
        }
}

void PipelineShaderStageCreateInfo::init_specialization_info(
        const std::vector<const Shader*>& shaders,
        const std::vector<const SpecializationConstant*>& constants)
{
        ASSERT(shaders.size() == constants.size());

        for (std::size_t i = 0; i < shaders.size(); ++i)
        {
                const SpecializationConstant* const constant = constants[i];

                if (!constant)
                {
                        continue;
                }

                specialization_info_.push_back(std::make_unique<VkSpecializationInfo>());

                VkSpecializationInfo& info = *specialization_info_.back();
                info = {};
                info.mapEntryCount = constant->entries().size();
                info.pMapEntries = constant->entries().data();
                info.dataSize = constant->size();
                info.pData = constant->data();

                create_info_[i].pSpecializationInfo = &info;
        }
}

PipelineShaderStageCreateInfo::PipelineShaderStageCreateInfo(
        const std::vector<const Shader*>& shaders,
        const std::vector<const SpecializationConstant*>* const constants)
{
        init_create_info(shaders);

        if (constants)
        {
                init_specialization_info(shaders, *constants);
        }
}

PipelineShaderStageCreateInfo::PipelineShaderStageCreateInfo(
        const std::vector<const Shader*>& shaders,
        const std::vector<const SpecializationConstant*>& constants)
{
        init_create_info(shaders);

        init_specialization_info(shaders, constants);
}
}
