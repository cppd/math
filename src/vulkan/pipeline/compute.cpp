/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "compute.h"

#include "shader_info.h"

#include <src/com/error.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

namespace ns::vulkan::pipeline
{
handle::Pipeline create_compute_pipeline(const ComputePipelineCreateInfo& info)
{
        if (info.device == VK_NULL_HANDLE || info.pipeline_layout == VK_NULL_HANDLE || !info.shader)
        {
                error("No required data to create compute pipeline");
        }

        ASSERT(info.shader->stage() == VK_SHADER_STAGE_COMPUTE_BIT);

        ASSERT(!info.constants || info.constants->dataSize > 0);
        ASSERT(!info.constants || info.constants->pData != nullptr);
        ASSERT(!info.constants || info.constants->mapEntryCount > 0);
        ASSERT(!info.constants || info.constants->pMapEntries != nullptr);

        const PipelineShaderStageCreateInfo shader_stage_info(info.shader, info.constants);

        const VkComputePipelineCreateInfo create_info = [&]
        {
                VkComputePipelineCreateInfo res = {};
                res.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
                res.stage = *shader_stage_info.data();
                res.layout = info.pipeline_layout;
                return res;
        }();

        return {info.device, create_info};
}
}
