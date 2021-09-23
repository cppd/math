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

#include <src/numerical/matrix.h>
#include <src/numerical/region.h>
#include <src/numerical/vec.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::dft
{
class ViewMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int IMAGE_BINDING = 1;
        static constexpr int DATA_BINDING = 0;

        vulkan::Descriptors descriptors_;
        std::vector<vulkan::BufferWithMemory> uniform_buffers_;

        struct Data
        {
                alignas(sizeof(Vector4f)) Vector3f background_color;
                alignas(sizeof(Vector4f)) Vector3f foreground_color;
                float brightness;
        };

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        ViewMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<uint32_t>& family_indices);

        ViewMemory(const ViewMemory&) = delete;
        ViewMemory& operator=(const ViewMemory&) = delete;
        ViewMemory& operator=(ViewMemory&&) = delete;

        ViewMemory(ViewMemory&&) = default;
        ~ViewMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        //

        void set_background_color(const Vector3f& background_color) const;
        void set_foreground_color(const Vector3f& foreground_color) const;
        void set_brightness(float brightness) const;
        void set_image(VkSampler sampler, const vulkan::ImageView& image) const;
};

struct ViewVertex
{
        Vector4f position;
        Vector2f texture_coordinates;

        static std::vector<VkVertexInputBindingDescription> binding_descriptions();
        static std::vector<VkVertexInputAttributeDescription> attribute_descriptions();
};

class ViewProgram final
{
        const vulkan::Device* device_;

        vulkan::DescriptorSetLayout descriptor_set_layout_;
        vulkan::PipelineLayout pipeline_layout_;
        vulkan::VertexShader vertex_shader_;
        vulkan::FragmentShader fragment_shader_;

public:
        explicit ViewProgram(const vulkan::Device* device);

        ViewProgram(const ViewProgram&) = delete;
        ViewProgram& operator=(const ViewProgram&) = delete;
        ViewProgram& operator=(ViewProgram&&) = delete;

        ViewProgram(ViewProgram&&) = default;
        ~ViewProgram() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkPipelineLayout pipeline_layout() const;
};
}
