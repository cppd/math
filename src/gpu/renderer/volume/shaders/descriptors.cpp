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
std::vector<VkDescriptorSetLayoutBinding> VolumeSharedMemory::descriptor_set_layout_bindings(const Flags& flags)
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OPACITY_0_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OPACITY_1_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OPACITY_2_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DRAWING_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = COORDINATES_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DEPTH_IMAGE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = GGX_F1_ALBEDO_COSINE_ROUGHNESS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = GGX_F1_ALBEDO_COSINE_WEIGHTED_AVERAGE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
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

        if (flags.acceleration_structure)
        {
                ASSERT(!flags.shadow_map);
                VkDescriptorSetLayoutBinding b = {};
                b.binding = ACCELERATION_STRUCTURE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
                b.descriptorCount = 1;
                b.stageFlags = flags.acceleration_structure;

                bindings.push_back(b);
        }

        if (flags.shadow_map)
        {
                ASSERT(!flags.acceleration_structure);
                VkDescriptorSetLayoutBinding b = {};
                b.binding = SHADOW_MAP_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = flags.shadow_map;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

VolumeSharedMemory::VolumeSharedMemory(
        const VkDevice device,
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
        buffer_info.buffer = drawing.handle();
        buffer_info.offset = 0;
        buffer_info.range = drawing.size();

        descriptors_.update_descriptor_set(0, DRAWING_BINDING, buffer_info);
}

void VolumeSharedMemory::set_coordinates(const vulkan::Buffer& coordinates) const
{
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = coordinates.handle();
        buffer_info.offset = 0;
        buffer_info.range = coordinates.size();

        descriptors_.update_descriptor_set(0, COORDINATES_BINDING, buffer_info);
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
                image_info.imageView = cosine_roughness.handle();
                image_info.sampler = sampler;

                infos.emplace_back(image_info);
                bindings.push_back(GGX_F1_ALBEDO_COSINE_ROUGHNESS_BINDING);
        }
        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                image_info.imageView = cosine_weighted_average.handle();
                image_info.sampler = sampler;

                infos.emplace_back(image_info);
                bindings.push_back(GGX_F1_ALBEDO_COSINE_WEIGHTED_AVERAGE_BINDING);
        }

        descriptors_.update_descriptor_set(0, bindings, infos);
}

void VolumeSharedMemory::set_opacity(const std::vector<const vulkan::ImageView*>& images) const
{
        ASSERT(images.size() == 2 || images.size() == 3);
        ASSERT(std::all_of(
                images.cbegin(), images.cend(),
                [](const vulkan::ImageView* const image)
                {
                        return image->has_usage(VK_IMAGE_USAGE_STORAGE_BIT);
                }));
        ASSERT(images[0]->format() == VK_FORMAT_R32G32B32A32_UINT);
        ASSERT(images[1]->format() == VK_FORMAT_R32G32B32A32_SFLOAT);
        if (images.size() == 3)
        {
                ASSERT(images[2]->format() == VK_FORMAT_R32G32B32A32_SFLOAT);
        }

        std::vector<vulkan::Descriptors::Info> infos;
        std::vector<std::uint32_t> bindings;

        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                image_info.imageView = images[0]->handle();

                infos.emplace_back(image_info);
                bindings.push_back(OPACITY_0_BINDING);
        }
        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                image_info.imageView = images[1]->handle();

                infos.emplace_back(image_info);
                bindings.push_back(OPACITY_1_BINDING);
        }
        if (images.size() == 3)
        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                image_info.imageView = images[2]->handle();

                infos.emplace_back(image_info);
                bindings.push_back(OPACITY_2_BINDING);
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
                image_info.imageView = heads.handle();

                infos.emplace_back(image_info);
                bindings.push_back(TRANSPARENCY_HEADS_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = nodes.handle();
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
        image_info.imageView = shadow_image.handle();
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
        const vulkan::Buffer& buffer_volume)
        : descriptors_(vulkan::Descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings))
{
        std::vector<vulkan::Descriptors::Info> infos;
        std::vector<std::uint32_t> bindings;

        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = buffer_coordinates.handle();
                buffer_info.offset = 0;
                buffer_info.range = buffer_coordinates.size();

                infos.emplace_back(buffer_info);
                bindings.push_back(BUFFER_COORDINATES_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = buffer_volume.handle();
                buffer_info.offset = 0;
                buffer_info.range = buffer_volume.size();

                infos.emplace_back(buffer_info);
                bindings.push_back(BUFFER_VOLUME_BINDING);
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

void VolumeImageMemory::set_image(const VkSampler sampler, const VkImageView image) const
{
        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = image;
        image_info.sampler = sampler;

        descriptors_.update_descriptor_set(0, IMAGE_BINDING, image_info);
}

void VolumeImageMemory::set_transfer_function(const VkSampler sampler, const VkImageView transfer_function) const
{
        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = transfer_function;
        image_info.sampler = sampler;

        descriptors_.update_descriptor_set(0, TRANSFER_FUNCTION_BINDING, image_info);
}
}
