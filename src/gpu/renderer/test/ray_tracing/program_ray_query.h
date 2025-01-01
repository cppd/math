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

#pragma once

#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <vector>

namespace ns::gpu::renderer::test
{
class RayQueryProgram final
{
        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::handle::Pipeline pipeline_;

public:
        RayQueryProgram(VkDevice device, unsigned local_size);

        RayQueryProgram(const RayQueryProgram&) = delete;
        RayQueryProgram& operator=(const RayQueryProgram&) = delete;
        RayQueryProgram& operator=(RayQueryProgram&&) = delete;

        RayQueryProgram(RayQueryProgram&&) = default;
        ~RayQueryProgram() = default;

        [[nodiscard]] VkPipeline pipeline() const;

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const;
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        [[nodiscard]] VkPipelineLayout pipeline_layout() const;
};
}
