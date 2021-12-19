/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/color/color.h>
#include <src/numerical/region.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::renderer
{
class PointsProgram final
{
        const vulkan::Device* device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_shared_;
        vulkan::handle::DescriptorSetLayout descriptor_set_layout_mesh_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::VertexShader vertex_shader_0d_;
        vulkan::VertexShader vertex_shader_1d_;
        vulkan::FragmentShader fragment_shader_;

public:
        explicit PointsProgram(const vulkan::Device* device);

        PointsProgram(const PointsProgram&) = delete;
        PointsProgram& operator=(const PointsProgram&) = delete;
        PointsProgram& operator=(PointsProgram&&) = delete;

        PointsProgram(PointsProgram&&) = default;
        ~PointsProgram() = default;

        vulkan::handle::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                VkPrimitiveTopology primitive_topology,
                const Region<2, int>& viewport,
                bool transparency) const;

        VkDescriptorSetLayout descriptor_set_layout_shared() const;
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_shared_bindings();

        VkDescriptorSetLayout descriptor_set_layout_mesh() const;
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_mesh_bindings();

        VkPipelineLayout pipeline_layout() const;
};
}
