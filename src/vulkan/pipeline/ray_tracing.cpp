/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "ray_tracing.h"

#include "shader_info.h"

#include <src/com/error.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <vector>

namespace ns::vulkan::pipeline
{
namespace
{
std::vector<VkRayTracingShaderGroupCreateInfoKHR> create_shader_group_info(const RayTracingPipelineCreateInfo& info)
{
        std::vector<VkRayTracingShaderGroupCreateInfoKHR> res = info.shader_groups;
        for (VkRayTracingShaderGroupCreateInfoKHR& v : res)
        {
                v.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                v.pNext = nullptr;

                ASSERT(v.generalShader == VK_SHADER_UNUSED_KHR || v.generalShader < info.shaders.size());
                ASSERT(v.closestHitShader == VK_SHADER_UNUSED_KHR || v.closestHitShader < info.shaders.size());
                ASSERT(v.anyHitShader == VK_SHADER_UNUSED_KHR || v.anyHitShader < info.shaders.size());
                ASSERT(v.intersectionShader == VK_SHADER_UNUSED_KHR || v.intersectionShader < info.shaders.size());
        }
        return res;
}
}

handle::Pipeline create_ray_tracing_pipeline(const RayTracingPipelineCreateInfo& info)
{
        if (info.device == VK_NULL_HANDLE || info.pipeline_layout == VK_NULL_HANDLE || info.shaders.empty()
            || info.shader_groups.empty())
        {
                error("No required data to create ray tracing pipeline");
        }

        const PipelineShaderStageCreateInfo shader_stage_info(info.shaders, info.constants);

        const std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_group_info = create_shader_group_info(info);

        const VkRayTracingPipelineCreateInfoKHR create_info = [&]
        {
                VkRayTracingPipelineCreateInfoKHR res = {};
                res.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
                res.stageCount = shader_stage_info.size();
                res.pStages = shader_stage_info.data();
                res.groupCount = shader_group_info.size();
                res.pGroups = shader_group_info.data();
                res.maxPipelineRayRecursionDepth = 1;
                res.layout = info.pipeline_layout;
                return res;
        }();

        return {info.device, create_info};
}
}
