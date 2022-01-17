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

#include <src/vulkan/descriptor.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vector>

namespace ns::gpu::renderer
{
class VolumeSharedMemory final
{
        static constexpr int SET_NUMBER = 0;

        static constexpr int DRAWING_BINDING = 0;
        static constexpr int DEPTH_IMAGE_BINDING = 1;
        static constexpr int GGX_F1_ALBEDO_COSINE_ROUGHNESS_BINDING = 2;
        static constexpr int GGX_F1_ALBEDO_COSINE_WEIGHTED_AVERAGE_BINDING = 3;
        static constexpr int TRANSPARENCY_HEADS_BINDING = 4;
        static constexpr int TRANSPARENCY_NODES_BINDING = 5;

        vulkan::Descriptors descriptors_;

public:
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();
        static unsigned set_number();

        VolumeSharedMemory(
                const vulkan::Device& device,
                VkDescriptorSetLayout descriptor_set_layout,
                const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
                const vulkan::Buffer& drawing,
                VkSampler ggx_f1_albedo_sampler,
                const vulkan::ImageView& ggx_f1_albedo_cosine_roughness,
                const vulkan::ImageView& ggx_f1_albedo_cosine_weighted_average);

        VolumeSharedMemory(const VolumeSharedMemory&) = delete;
        VolumeSharedMemory& operator=(const VolumeSharedMemory&) = delete;
        VolumeSharedMemory& operator=(VolumeSharedMemory&&) = delete;

        VolumeSharedMemory(VolumeSharedMemory&&) = default;
        ~VolumeSharedMemory() = default;

        //

        const VkDescriptorSet& descriptor_set() const;

        void set_depth_image(VkImageView image_view, VkSampler sampler) const;
        void set_transparency(const vulkan::ImageView& heads, const vulkan::Buffer& nodes) const;
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
}