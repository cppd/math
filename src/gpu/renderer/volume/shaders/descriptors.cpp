/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/error.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <vector>

namespace ns::gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> VolumeSharedMemory::descriptor_set_layout_bindings(const Flags& flags)
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(13);

        bindings.push_back(
                {.binding = OPACITY_0_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = OPACITY_1_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = OPACITY_2_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = OPACITY_3_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = DRAWING_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = COORDINATES_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = DEPTH_IMAGE_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = GGX_F1_ALBEDO_COSINE_ROUGHNESS_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = GGX_F1_ALBEDO_COSINE_WEIGHTED_AVERAGE_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = TRANSPARENCY_HEADS_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = TRANSPARENCY_NODES_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        if (flags.acceleration_structure)
        {
                ASSERT(!flags.shadow_map);
                bindings.push_back(
                        {.binding = ACCELERATION_STRUCTURE_BINDING,
                         .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
                         .descriptorCount = 1,
                         .stageFlags = flags.acceleration_structure,
                         .pImmutableSamplers = nullptr});
        }

        if (flags.shadow_map)
        {
                ASSERT(!flags.acceleration_structure);
                bindings.push_back(
                        {.binding = SHADOW_MAP_BINDING,
                         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                         .descriptorCount = 1,
                         .stageFlags = flags.shadow_map,
                         .pImmutableSamplers = nullptr});
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
        descriptors_.update_descriptor_set(
                0, DRAWING_BINDING,
                VkDescriptorBufferInfo{.buffer = drawing.handle(), .offset = 0, .range = drawing.size()});
}

void VolumeSharedMemory::set_coordinates(const vulkan::Buffer& coordinates) const
{
        descriptors_.update_descriptor_set(
                0, COORDINATES_BINDING,
                VkDescriptorBufferInfo{.buffer = coordinates.handle(), .offset = 0, .range = coordinates.size()});
}

void VolumeSharedMemory::set_ggx_f1_albedo(
        const VkSampler sampler,
        const vulkan::ImageView& cosine_roughness,
        const vulkan::ImageView& cosine_weighted_average) const
{
        static constexpr unsigned DESCRIPTOR_INDEX = 0;

        ASSERT(cosine_roughness.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));
        ASSERT(cosine_roughness.sample_count() == VK_SAMPLE_COUNT_1_BIT);
        ASSERT(cosine_weighted_average.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));
        ASSERT(cosine_weighted_average.sample_count() == VK_SAMPLE_COUNT_1_BIT);

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        infos.emplace_back(
                DESCRIPTOR_INDEX, GGX_F1_ALBEDO_COSINE_ROUGHNESS_BINDING,
                VkDescriptorImageInfo{
                        .sampler = sampler,
                        .imageView = cosine_roughness.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

        infos.emplace_back(
                DESCRIPTOR_INDEX, GGX_F1_ALBEDO_COSINE_WEIGHTED_AVERAGE_BINDING,
                VkDescriptorImageInfo{
                        .sampler = sampler,
                        .imageView = cosine_weighted_average.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

        descriptors_.update_descriptor_set(infos);
}

void VolumeSharedMemory::set_opacity(const std::vector<const vulkan::ImageView*>& images) const
{
        static constexpr unsigned DESCRIPTOR_INDEX = 0;

        ASSERT(images.size() == 2 || images.size() == 4);
        ASSERT(std::all_of(
                images.cbegin(), images.cend(),
                [](const vulkan::ImageView* const image)
                {
                        return image->has_usage(VK_IMAGE_USAGE_STORAGE_BIT);
                }));
        ASSERT(images[0]->format() == VK_FORMAT_R32G32_UINT);
        ASSERT(images[1]->format() == VK_FORMAT_R32G32B32A32_SFLOAT);
        if (images.size() == 4)
        {
                ASSERT(images[2]->format() == VK_FORMAT_R32G32B32A32_SFLOAT);
                ASSERT(images[3]->format() == VK_FORMAT_R32G32_SFLOAT);
        }

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(images.size());

        infos.emplace_back(
                DESCRIPTOR_INDEX, OPACITY_0_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = images[0]->handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

        infos.emplace_back(
                DESCRIPTOR_INDEX, OPACITY_1_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = images[1]->handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

        if (images.size() == 4)
        {
                infos.emplace_back(
                        DESCRIPTOR_INDEX, OPACITY_2_BINDING,
                        VkDescriptorImageInfo{
                                .sampler = VK_NULL_HANDLE,
                                .imageView = images[2]->handle(),
                                .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

                infos.emplace_back(
                        DESCRIPTOR_INDEX, OPACITY_3_BINDING,
                        VkDescriptorImageInfo{
                                .sampler = VK_NULL_HANDLE,
                                .imageView = images[3]->handle(),
                                .imageLayout = VK_IMAGE_LAYOUT_GENERAL});
        }

        descriptors_.update_descriptor_set(infos);
}

void VolumeSharedMemory::set_depth_image(const VkImageView image_view, const VkSampler sampler) const
{
        descriptors_.update_descriptor_set(
                0, DEPTH_IMAGE_BINDING,
                VkDescriptorImageInfo{
                        .sampler = sampler,
                        .imageView = image_view,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
}

void VolumeSharedMemory::set_transparency(const vulkan::ImageView& heads, const vulkan::Buffer& nodes) const
{
        static constexpr unsigned DESCRIPTOR_INDEX = 0;

        ASSERT(heads.format() == VK_FORMAT_R32_UINT);
        ASSERT(heads.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(nodes.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        infos.emplace_back(
                DESCRIPTOR_INDEX, TRANSPARENCY_HEADS_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = heads.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

        infos.emplace_back(
                DESCRIPTOR_INDEX, TRANSPARENCY_NODES_BINDING,
                VkDescriptorBufferInfo{.buffer = nodes.handle(), .offset = 0, .range = nodes.size()});

        descriptors_.update_descriptor_set(infos);
}

void VolumeSharedMemory::set_shadow_image(const VkSampler sampler, const vulkan::ImageView& shadow_image) const
{
        ASSERT(shadow_image.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));
        ASSERT(shadow_image.sample_count() == VK_SAMPLE_COUNT_1_BIT);

        descriptors_.update_descriptor_set(
                0, SHADOW_MAP_BINDING,
                VkDescriptorImageInfo{
                        .sampler = sampler,
                        .imageView = shadow_image.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
}

void VolumeSharedMemory::set_acceleration_structure(const VkAccelerationStructureKHR acceleration_structure) const
{
        descriptors_.update_descriptor_set(0, ACCELERATION_STRUCTURE_BINDING, acceleration_structure);
}

//

std::vector<VkDescriptorSetLayoutBinding> VolumeImageMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(4);

        bindings.push_back(
                {.binding = BUFFER_COORDINATES_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = BUFFER_VOLUME_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = IMAGE_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = TRANSFER_FUNCTION_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

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
        static constexpr unsigned DESCRIPTOR_INDEX = 0;

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2);

        infos.emplace_back(
                DESCRIPTOR_INDEX, BUFFER_COORDINATES_BINDING,
                VkDescriptorBufferInfo{
                        .buffer = buffer_coordinates.handle(),
                        .offset = 0,
                        .range = buffer_coordinates.size()});

        infos.emplace_back(
                DESCRIPTOR_INDEX, BUFFER_VOLUME_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer_volume.handle(), .offset = 0, .range = buffer_volume.size()});

        descriptors_.update_descriptor_set(infos);
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
        descriptors_.update_descriptor_set(
                0, IMAGE_BINDING,
                VkDescriptorImageInfo{
                        .sampler = sampler,
                        .imageView = image,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
}

void VolumeImageMemory::set_transfer_function(const VkSampler sampler, const VkImageView transfer_function) const
{
        descriptors_.update_descriptor_set(
                0, TRANSFER_FUNCTION_BINDING,
                VkDescriptorImageInfo{
                        .sampler = sampler,
                        .imageView = transfer_function,
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
}
}
