/*
Copyright (C) 2017-2020 Topological Manifold

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
#include <src/numerical/vec.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace gpu::renderer
{
class VolumeMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int VOLUME_BINDING = 0;
        static constexpr int DRAWING_BINDING = 1;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        VolumeMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const vulkan::Buffer& volume,
                const vulkan::Buffer& drawing);

        VolumeMemory(const VolumeMemory&) = delete;
        VolumeMemory& operator=(const VolumeMemory&) = delete;
        VolumeMemory& operator=(VolumeMemory&&) = delete;

        VolumeMemory(VolumeMemory&&) = default;
        ~VolumeMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;
};

class VolumeImageMemory final
{
        static constexpr int SET_NUMBER = 1;

        static constexpr int IMAGE_BINDING = 0;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        VolumeImageMemory(
                VkDevice device,
                VkSampler sampler,
                VkDescriptorSetLayout descriptor_set_layout,
                VkImageView image_view);

        VolumeImageMemory(const VolumeImageMemory&) = delete;
        VolumeImageMemory& operator=(const VolumeImageMemory&) = delete;
        VolumeImageMemory& operator=(VolumeImageMemory&&) = delete;

        VolumeImageMemory(VolumeImageMemory&&) = default;
        ~VolumeImageMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;
        VkDescriptorSetLayout descriptor_set_layout() const;
};

class VolumeProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::DescriptorSetLayout m_descriptor_set_layout_image;
        vulkan::PipelineLayout m_pipeline_layout;
        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader;

public:
        explicit VolumeProgram(const vulkan::Device& device);

        VolumeProgram(const VolumeProgram&) = delete;
        VolumeProgram& operator=(const VolumeProgram&) = delete;
        VolumeProgram& operator=(VolumeProgram&&) = delete;

        VolumeProgram(VolumeProgram&&) = default;
        ~VolumeProgram() = default;

        vulkan::Pipeline create_pipeline(
                VkRenderPass render_pass,
                VkSampleCountFlagBits sample_count,
                bool sample_shading,
                const Region<2, int>& viewport) const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        VkDescriptorSetLayout descriptor_set_layout_image() const;
        VkPipelineLayout pipeline_layout() const;
};
}
