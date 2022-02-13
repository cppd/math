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

#include <src/vulkan/constant.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vector>

namespace ns::gpu::renderer
{
class SharedConstants final : public vulkan::SpecializationConstant
{
        struct Data
        {
                std::uint32_t transparency_drawing;
        } data_;

        std::vector<VkSpecializationMapEntry> entries_;

        const std::vector<VkSpecializationMapEntry>& entries() const override;
        const void* data() const override;
        std::size_t size() const override;

public:
        SharedConstants();

        void set(bool transparency_drawing);
};

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
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(
                VkShaderStageFlags shadow_matrices,
                VkShaderStageFlags drawing,
                VkShaderStageFlags objects,
                VkShaderStageFlags shadow_map,
                VkShaderStageFlags acceleration_structure);
        static unsigned set_number();

        SharedMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const vulkan::Buffer& shadow_matrices,
                const vulkan::Buffer& drawing,
                VkSampler ggx_f1_albedo_sampler,
                const vulkan::ImageView& ggx_f1_albedo_cosine_roughness,
                const vulkan::ImageView& ggx_f1_albedo_cosine_weighted_average);

        const VkDescriptorSet& descriptor_set() const;

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

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(VkShaderStageFlags coordinates);
        static unsigned set_number();

        static vulkan::Descriptors create(
                VkDevice device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const std::vector<const vulkan::Buffer*>& coordinates);
};

class MaterialMemory final
{
        static constexpr int SET_NUMBER = 2;

        static constexpr int MATERIAL_BINDING = 0;
        static constexpr int TEXTURE_BINDING = 1;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        struct MaterialInfo final
        {
                VkBuffer buffer;
                VkDeviceSize buffer_size;
                VkImageView texture;
        };

        static vulkan::Descriptors create(
                VkDevice device,
                VkSampler sampler,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const std::vector<MaterialInfo>& materials);
};
}
