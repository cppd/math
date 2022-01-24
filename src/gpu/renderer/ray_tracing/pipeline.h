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

#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::renderer
{
class RayTracingPipeline final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::Shader ray_closest_hit_shader_;
        vulkan::Shader ray_generation_shader_;
        vulkan::Shader ray_miss_shader_;

public:
        explicit RayTracingPipeline(VkDevice device);

        RayTracingPipeline(const RayTracingPipeline&) = delete;
        RayTracingPipeline& operator=(const RayTracingPipeline&) = delete;
        RayTracingPipeline& operator=(RayTracingPipeline&&) = delete;

        RayTracingPipeline(RayTracingPipeline&&) = default;
        ~RayTracingPipeline() = default;

        vulkan::handle::Pipeline create_pipeline() const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        VkPipelineLayout pipeline_layout() const;
};
}
