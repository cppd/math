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

#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <vector>

namespace ns::gpu::renderer
{
class VolumeSharedMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int OPACITY_0_BINDING = 0;
        static constexpr int OPACITY_1_BINDING = 1;
        static constexpr int OPACITY_2_BINDING = 2;
        static constexpr int OPACITY_3_BINDING = 3;

        static constexpr int TRANSPARENCY_HEADS_BINDING = 4;
        static constexpr int TRANSPARENCY_NODES_BINDING = 5;

        static constexpr int COORDINATES_BINDING = 6;

        static constexpr int DEPTH_IMAGE_BINDING = 7;

        static constexpr int DRAWING_BINDING = 8;

        static constexpr int GGX_F1_ALBEDO_COSINE_ROUGHNESS_BINDING = 9;
        static constexpr int GGX_F1_ALBEDO_COSINE_WEIGHTED_AVERAGE_BINDING = 10;

        static constexpr int ACCELERATION_STRUCTURE_BINDING = 11;
        static constexpr int SHADOW_MAP_BINDING = 11;

        vulkan::Descriptors descriptors_;

public:
        struct Flags final
        {
                VkShaderStageFlags shadow_map;
                VkShaderStageFlags acceleration_structure;
        };

        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(
                const Flags& flags);
        [[nodiscard]] static unsigned set_number();

        VolumeSharedMemory(
                VkDevice device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings);

        VolumeSharedMemory(const VolumeSharedMemory&) = delete;
        VolumeSharedMemory& operator=(const VolumeSharedMemory&) = delete;
        VolumeSharedMemory& operator=(VolumeSharedMemory&&) = delete;

        VolumeSharedMemory(VolumeSharedMemory&&) = default;
        ~VolumeSharedMemory() = default;

        //

        [[nodiscard]] const VkDescriptorSet& descriptor_set() const;

        void set_drawing(const vulkan::Buffer& drawing) const;
        void set_coordinates(const vulkan::Buffer& coordinates) const;

        void set_ggx_f1_albedo(
                VkSampler sampler,
                const vulkan::ImageView& cosine_roughness,
                const vulkan::ImageView& cosine_weighted_average) const;

        void set_opacity(const std::vector<const vulkan::ImageView*>& images) const;

        void set_depth_image(VkImageView image_view, VkSampler sampler) const;
        void set_transparency(const vulkan::ImageView& heads, const vulkan::Buffer& nodes) const;

        void set_shadow_image(VkSampler sampler, const vulkan::ImageView& shadow_image) const;
        void set_acceleration_structure(VkAccelerationStructureKHR acceleration_structure) const;
};

class VolumeImageMemory final
{
        static constexpr int SET_NUMBER = 1;

        static constexpr int BUFFER_COORDINATES_BINDING = 0;
        static constexpr int BUFFER_VOLUME_BINDING = 1;

        static constexpr int IMAGE_BINDING = 2;
        static constexpr int TRANSFER_FUNCTION_BINDING = 3;

        vulkan::Descriptors descriptors_;

public:
        [[nodiscard]] static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        [[nodiscard]] static unsigned set_number();

        VolumeImageMemory(
                VkDevice device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const vulkan::Buffer& buffer_coordinates,
                const vulkan::Buffer& buffer_volume);

        [[nodiscard]] const VkDescriptorSet& descriptor_set() const;

        void set_image(VkSampler sampler, VkImageView image) const;
        void set_transfer_function(VkSampler sampler, VkImageView transfer_function) const;
};
}
