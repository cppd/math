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

#include "../../code/code.h"

#include <src/numerical/region.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::renderer
{
class FragmentsProgram final
{
        const vulkan::Device* device_;
        bool ray_tracing_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_shared_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::Shader vertex_shader_;
        vulkan::Shader fragment_shader_;

public:
        explicit FragmentsProgram(const vulkan::Device* device, const Code& code);

        FragmentsProgram(const FragmentsProgram&) = delete;
        FragmentsProgram& operator=(const FragmentsProgram&) = delete;
        FragmentsProgram& operator=(FragmentsProgram&&) = delete;

        FragmentsProgram(FragmentsProgram&&) = default;
        ~FragmentsProgram() = default;

        vulkan::handle::Pipeline create_pipeline(
                const vulkan::RenderPass& render_pass,
                VkSampleCountFlagBits sample_count,
                bool sample_shading,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout_shared() const;
        std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_shared_bindings() const;

        VkPipelineLayout pipeline_layout() const;
};
}
