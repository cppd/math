/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>

#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace ns::gpu::renderer
{
std::vector<VkPushConstantRange> push_constant_ranges();

void push_constant_command(VkCommandBuffer command_buffer, VkPipelineLayout pipeline_layout, bool transparency_drawing);

//

class SharedMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DRAWING_BINDING = 0;
        static constexpr int SHADOW_MATRICES_BINDING = 1;

        static constexpr int ACCELERATION_STRUCTURE_BINDING = 2;
        static constexpr int SHADOW_MAP_BINDING = 2;

        static constexpr int OBJECTS_BINDING = 3;

        static constexpr int GGX_F1_ALBEDO_COSINE_ROUGHNESS_BINDING = 4;
        static constexpr int GGX_F1_ALBEDO_COSINE_WEIGHTED_AVERAGE_BINDING = 5;

        static constexpr int TRANSPARENCY_HEADS_BINDING = 6;
        static constexpr int TRANSPARENCY_HEADS_SIZE_BINDING = 7;
        static constexpr int TRANSPARENCY_COUNTERS_BINDING = 8;
        static constexpr int TRANSPARENCY_NODES_BINDING = 9;

        vulkan::Descriptors descriptors_;

public:
        struct Flags final
        {
                VkShaderStageFlags shadow_matrices;
                VkShaderStageFlags drawing;
                VkShaderStageFlags objects;
                VkShaderStageFlags shadow_map;
                VkShaderStageFlags acceleration_structure;
                VkShaderStageFlags ggx_f1_albedo;
                VkShaderStageFlags transparency;
        };

        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(
                const Flags& flags);
        [[nodiscard]] static unsigned set_number();

        SharedMemory(
                VkDevice device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const vulkan::Buffer& drawing);

        [[nodiscard]] const VkDescriptorSet& descriptor_set() const;

        void set_shadow_matrices(const vulkan::Buffer& shadow_matrices) const;

        void set_ggx_f1_albedo(
                VkSampler sampler,
                const vulkan::ImageView& cosine_roughness,
                const vulkan::ImageView& cosine_weighted_average) const;

        void set_objects_image(const vulkan::ImageView& objects_image) const;

        void set_transparency(
                const vulkan::ImageView& heads,
                const vulkan::ImageView& heads_size,
                const vulkan::Buffer& counters,
                const vulkan::Buffer& nodes) const;

        void set_shadow_image(VkSampler sampler, const vulkan::ImageView& shadow_image) const;
        void set_acceleration_structure(VkAccelerationStructureKHR acceleration_structure) const;
};

class MeshMemory final
{
        static constexpr int SET_NUMBER = 1;

        static constexpr int BUFFER_BINDING = 0;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(
                VkShaderStageFlags coordinates);
        [[nodiscard]] static unsigned set_number();

        MeshMemory(
                VkDevice device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const vulkan::Buffer& buffer);

        [[nodiscard]] const VkDescriptorSet& descriptor_set() const;
};

class MaterialMemory final
{
        static constexpr int SET_NUMBER = 2;

        static constexpr int MATERIAL_BINDING = 0;
        static constexpr int TEXTURE_BINDING = 1;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        struct MaterialInfo final
        {
                VkBuffer buffer;
                VkDeviceSize buffer_size;
                VkImageView texture;
        };

        MaterialMemory(
                VkDevice device,
                VkSampler sampler,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const std::vector<MaterialInfo>& materials);

        [[nodiscard]] std::uint32_t descriptor_set_count() const;
        [[nodiscard]] const VkDescriptorSet& descriptor_set(std::uint32_t index) const;
};
}
