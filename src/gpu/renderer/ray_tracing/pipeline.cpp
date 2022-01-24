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

#include "pipeline.h"

#include "descriptors.h"

#include "code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> RayTracingPipeline::descriptor_set_layout_bindings()
{
        return RayTracingMemory::descriptor_set_layout_bindings();
}

RayTracingPipeline::RayTracingPipeline(const VkDevice device)
        : device_(device),
          descriptor_set_layout_(vulkan::create_descriptor_set_layout(device_, descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device_, {RayTracingMemory::set_number()}, {descriptor_set_layout_})),
          ray_closest_hit_shader_(device_, code_ray_closest_hit_rchit(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR),
          ray_generation_shader_(device_, code_ray_generation_rgen(), VK_SHADER_STAGE_RAYGEN_BIT_KHR),
          ray_miss_shader_(device_, code_ray_miss_rmiss(), VK_SHADER_STAGE_MISS_BIT_KHR)
{
}

VkDescriptorSetLayout RayTracingPipeline::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout RayTracingPipeline::pipeline_layout() const
{
        return pipeline_layout_;
}

vulkan::handle::Pipeline RayTracingPipeline::create_pipeline() const
{
        std::vector<const vulkan::Shader*> shaders(3);

        shaders[0] = &ray_closest_hit_shader_;
        shaders[1] = &ray_generation_shader_;
        shaders[2] = &ray_miss_shader_;

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups(3);

        shader_groups[0] = {};
        shader_groups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        shader_groups[0].generalShader = VK_SHADER_UNUSED_KHR;
        shader_groups[0].closestHitShader = 0;
        shader_groups[0].anyHitShader = VK_SHADER_UNUSED_KHR;
        shader_groups[0].intersectionShader = VK_SHADER_UNUSED_KHR;

        shader_groups[1] = {};
        shader_groups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shader_groups[1].generalShader = 1;
        shader_groups[1].closestHitShader = VK_SHADER_UNUSED_KHR;
        shader_groups[1].anyHitShader = VK_SHADER_UNUSED_KHR;
        shader_groups[1].intersectionShader = VK_SHADER_UNUSED_KHR;

        shader_groups[2] = {};
        shader_groups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shader_groups[2].generalShader = 2;
        shader_groups[2].closestHitShader = VK_SHADER_UNUSED_KHR;
        shader_groups[2].anyHitShader = VK_SHADER_UNUSED_KHR;
        shader_groups[2].intersectionShader = VK_SHADER_UNUSED_KHR;

        vulkan::RayTracingPipelineCreateInfo info;
        info.device = device_;
        info.pipeline_layout = pipeline_layout_;
        info.shaders = &shaders;
        info.shader_groups = &shader_groups;

        return vulkan::create_ray_tracing_pipeline(info);
}
}
