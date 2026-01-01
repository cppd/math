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

#pragma once

#include <src/numerical/region.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vulkan/vulkan_core.h>

#include <vector>

namespace ns::gpu::pencil_sketch
{
class ComputeMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int INPUT_BINDING = 0;
        static constexpr int OUTPUT_BINDING = 1;
        static constexpr int OBJECTS_BINDING = 2;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        ComputeMemory(VkDevice device, VkDescriptorSetLayout descriptor_set_layout);

        [[nodiscard]] const VkDescriptorSet& descriptor_set() const;

        void set_input(VkSampler sampler, const vulkan::ImageView& image) const;
        void set_output_image(const vulkan::ImageView& image) const;
        void set_object_image(const vulkan::ImageView& image) const;
};

class ComputeProgram final
{
        VkDevice device_;

        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::Shader shader_;
        vulkan::handle::Pipeline pipeline_;

public:
        explicit ComputeProgram(VkDevice device);

        ComputeProgram(const ComputeProgram&) = delete;
        ComputeProgram& operator=(const ComputeProgram&) = delete;
        ComputeProgram& operator=(ComputeProgram&&) = delete;

        ComputeProgram(ComputeProgram&&) = default;
        ~ComputeProgram() = default;

        void create_pipeline(unsigned group_size, const numerical::Region<2, int>& rectangle);
        void delete_pipeline();

        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout() const;
        [[nodiscard]] VkPipelineLayout pipeline_layout() const;
        [[nodiscard]] VkPipeline pipeline() const;
};
}
