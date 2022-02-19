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

#include "descriptors.h"

namespace ns::gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> VolumeSharedMemory::descriptor_set_layout_bindings(
        const VkShaderStageFlags drawing,
        const VkShaderStageFlags depth_image,
        const VkShaderStageFlags ggx_f1_albedo,
        const VkShaderStageFlags shadow_map,
        const VkShaderStageFlags acceleration_structure)
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        if (drawing)
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DRAWING_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = drawing;

                bindings.push_back(b);
        }

        if (depth_image)
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DEPTH_IMAGE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = depth_image;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        if (ggx_f1_albedo)
        {
                {
                        VkDescriptorSetLayoutBinding b = {};
                        b.binding = GGX_F1_ALBEDO_COSINE_ROUGHNESS_BINDING;
                        b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        b.descriptorCount = 1;
                        b.stageFlags = ggx_f1_albedo;
                        b.pImmutableSamplers = nullptr;

                        bindings.push_back(b);
                }
                {
                        VkDescriptorSetLayoutBinding b = {};
                        b.binding = GGX_F1_ALBEDO_COSINE_WEIGHTED_AVERAGE_BINDING;
                        b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        b.descriptorCount = 1;
                        b.stageFlags = ggx_f1_albedo;
                        b.pImmutableSamplers = nullptr;

                        bindings.push_back(b);
                }
        }

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TRANSPARENCY_HEADS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TRANSPARENCY_NODES_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }

        if (acceleration_structure)
        {
                ASSERT(!shadow_map);
                VkDescriptorSetLayoutBinding b = {};
                b.binding = ACCELERATION_STRUCTURE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
                b.descriptorCount = 1;
                b.stageFlags = acceleration_structure;

                bindings.push_back(b);
        }

        if (shadow_map)
        {
                ASSERT(!acceleration_structure);
                VkDescriptorSetLayoutBinding b = {};
                b.binding = SHADOW_MAP_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = shadow_map;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

VolumeSharedMemory::VolumeSharedMemory(
        const vulkan::Device& device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings)
{
}

unsigned VolumeSharedMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& VolumeSharedMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void VolumeSharedMemory::set_drawing(const vulkan::Buffer& drawing) const
{
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = drawing;
        buffer_info.offset = 0;
        buffer_info.range = drawing.size();

        descriptors_.update_descriptor_set(0, DRAWING_BINDING, buffer_info);
}

void VolumeSharedMemory::set_ggx_f1_albedo(
        const VkSampler sampler,
        const vulkan::ImageView& cosine_roughness,
        const vulkan::ImageView& cosine_weighted_average) const
{
        ASSERT(cosine_roughness.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));
        ASSERT(cosine_roughness.sample_count() == VK_SAMPLE_COUNT_1_BIT);
        ASSERT(cosine_weighted_average.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));
        ASSERT(cosine_weighted_average.sample_count() == VK_SAMPLE_COUNT_1_BIT);

        std::vector<vulkan::Descriptors::Info> infos;
        std::vector<std::uint32_t> bindings;

        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_info.imageView = cosine_roughness;
                image_info.sampler = sampler;

                infos.emplace_back(image_info);
                bindings.push_back(GGX_F1_ALBEDO_COSINE_ROUGHNESS_BINDING);
        }
        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_info.imageView = cosine_weighted_average;
                image_info.sampler = sampler;

                infos.emplace_back(image_info);
                bindings.push_back(GGX_F1_ALBEDO_COSINE_WEIGHTED_AVERAGE_BINDING);
        }

        descriptors_.update_descriptor_set(0, bindings, infos);
}

void VolumeSharedMemory::set_depth_image(const VkImageView image_view, const VkSampler sampler) const
{
        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image_view;
        image_info.sampler = sampler;

        descriptors_.update_descriptor_set(0, DEPTH_IMAGE_BINDING, image_info);
}

void VolumeSharedMemory::set_transparency(const vulkan::ImageView& heads, const vulkan::Buffer& nodes) const
{
        ASSERT(heads.format() == VK_FORMAT_R32_UINT);
        ASSERT(heads.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(nodes.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        std::vector<vulkan::Descriptors::Info> infos;
        std::vector<std::uint32_t> bindings;

        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                image_info.imageView = heads;

                infos.emplace_back(image_info);
                bindings.push_back(TRANSPARENCY_HEADS_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = nodes;
                buffer_info.offset = 0;
                buffer_info.range = nodes.size();

                infos.emplace_back(buffer_info);
                bindings.push_back(TRANSPARENCY_NODES_BINDING);
        }

        descriptors_.update_descriptor_set(0, bindings, infos);
}

void VolumeSharedMemory::set_shadow_image(const VkSampler sampler, const vulkan::ImageView& shadow_image) const
{
        ASSERT(shadow_image.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));
        ASSERT(shadow_image.sample_count() == VK_SAMPLE_COUNT_1_BIT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = shadow_image;
        image_info.sampler = sampler;

        descriptors_.update_descriptor_set(0, SHADOW_MAP_BINDING, image_info);
}

void VolumeSharedMemory::set_acceleration_structure(const VkAccelerationStructureKHR acceleration_structure) const
{
        descriptors_.update_descriptor_set(0, ACCELERATION_STRUCTURE_BINDING, acceleration_structure);
}

//

std::vector<VkDescriptorSetLayoutBinding> VolumeImageMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_COORDINATES_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_VOLUME_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = IMAGE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TRANSFER_FUNCTION_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

VolumeImageMemory::VolumeImageMemory(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const vulkan::Buffer& buffer_coordinates,
        const vulkan::Buffer& buffer_volume,
        const VkSampler image_sampler,
        const vulkan::ImageView& image,
        const VkSampler transfer_function_sampler,
        const vulkan::ImageView& transfer_function)
        : descriptors_(vulkan::Descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings))
{
        std::vector<vulkan::Descriptors::Info> infos;
        std::vector<std::uint32_t> bindings;

        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = buffer_coordinates;
                buffer_info.offset = 0;
                buffer_info.range = buffer_coordinates.size();

                infos.emplace_back(buffer_info);
                bindings.push_back(BUFFER_COORDINATES_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = buffer_volume;
                buffer_info.offset = 0;
                buffer_info.range = buffer_volume.size();

                infos.emplace_back(buffer_info);
                bindings.push_back(BUFFER_VOLUME_BINDING);
        }
        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_info.imageView = image;
                image_info.sampler = image_sampler;

                infos.emplace_back(image_info);
                bindings.push_back(IMAGE_BINDING);
        }
        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_info.imageView = transfer_function;
                image_info.sampler = transfer_function_sampler;

                infos.emplace_back(image_info);
                bindings.push_back(TRANSFER_FUNCTION_BINDING);
        }

        descriptors_.update_descriptor_set(0, bindings, infos);
}

unsigned VolumeImageMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& VolumeImageMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}
}
