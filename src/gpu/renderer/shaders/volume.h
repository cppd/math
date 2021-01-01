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

#include "buffers.h"

#include <src/color/color.h>
#include <src/numerical/region.h>
#include <src/numerical/vec.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>
#include <src/vulkan/shader.h>

#include <vector>

namespace ns::gpu::renderer
{
class VolumeSharedMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DRAWING_BINDING = 0;
        static constexpr int DEPTH_IMAGE_BINDING = 1;
        static constexpr int TRANSPARENCY_HEADS_BINDING = 2;
        static constexpr int TRANSPARENCY_NODES_BINDING = 3;

        vulkan::Descriptors m_descriptors;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        VolumeSharedMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const vulkan::Buffer& drawing);

        VolumeSharedMemory(const VolumeSharedMemory&) = delete;
        VolumeSharedMemory& operator=(const VolumeSharedMemory&) = delete;
        VolumeSharedMemory& operator=(VolumeSharedMemory&&) = delete;

        VolumeSharedMemory(VolumeSharedMemory&&) = default;
        ~VolumeSharedMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        void set_depth_image(VkImageView image_view, VkSampler sampler) const;
        void set_transparency(const vulkan::ImageWithMemory& heads, const vulkan::Buffer& nodes) const;
};

class VolumeImageMemory final
{
        static constexpr int SET_NUMBER = 1;

        static constexpr int BUFFER_COORDINATES_BINDING = 0;
        static constexpr int BUFFER_VOLUME_BINDING = 1;
        static constexpr int IMAGE_BINDING = 2;
        static constexpr int TRANSFER_FUNCTION_BINDING = 3;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        struct CreateInfo final
        {
                VkBuffer buffer_coordinates;
                VkDeviceSize buffer_coordinates_size;
                VkBuffer buffer_volume;
                VkDeviceSize buffer_volume_size;
                VkImageView image;
                VkImageView transfer_function;
        };

        static vulkan::Descriptors create(
                VkDevice device,
                VkSampler image_sampler,
                VkSampler transfer_function_sampler,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const CreateInfo& create_info);
};

class VolumeProgram final
{
        const vulkan::Device& m_device;

        vulkan::DescriptorSetLayout m_descriptor_set_layout_shared;
        vulkan::DescriptorSetLayout m_descriptor_set_layout_image;
        vulkan::PipelineLayout m_pipeline_layout_image_fragments;
        vulkan::PipelineLayout m_pipeline_layout_fragments;
        vulkan::VertexShader m_vertex_shader;
        vulkan::FragmentShader m_fragment_shader_image;
        vulkan::FragmentShader m_fragment_shader_image_fragments;
        vulkan::FragmentShader m_fragment_shader_fragments;

public:
        enum class PipelineLayoutType
        {
                ImageFragments,
                Fragments
        };

        enum class PipelineType
        {
                Image,
                ImageFragments,
                Fragments
        };

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
                const Region<2, int>& viewport,
                PipelineType type) const;

        VkDescriptorSetLayout descriptor_set_layout_shared() const;
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_shared_bindings();

        VkDescriptorSetLayout descriptor_set_layout_image() const;
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_image_bindings();

        VkPipelineLayout pipeline_layout(PipelineLayoutType type) const;
};
}
