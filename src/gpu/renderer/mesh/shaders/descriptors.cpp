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
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
namespace
{
constexpr VkShaderStageFlags PUSH_CONSTANT_STAGE = VK_SHADER_STAGE_FRAGMENT_BIT;

struct PushConstants final
{
        std::uint32_t transparency_drawing;
};
}

//

std::vector<VkPushConstantRange> push_constant_ranges()
{
        return {
                {.stageFlags = PUSH_CONSTANT_STAGE, .offset = 0, .size = sizeof(PushConstants)}
        };
}

void push_constant_command(
        const VkCommandBuffer command_buffer,
        const VkPipelineLayout pipeline_layout,
        const bool transparency_drawing)
{
        const PushConstants values{.transparency_drawing = transparency_drawing ? 1u : 0u};

        vkCmdPushConstants(command_buffer, pipeline_layout, PUSH_CONSTANT_STAGE, 0, sizeof(PushConstants), &values);
}

//

std::vector<VkDescriptorSetLayoutBinding> SharedMemory::descriptor_set_layout_bindings(const Flags& flags)
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(11);

        bindings.push_back(
                {.binding = DRAWING_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = flags.drawing | VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        if (flags.shadow_matrices)
        {
                bindings.push_back(
                        {.binding = SHADOW_MATRICES_BINDING,
                         .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                         .descriptorCount = 1,
                         .stageFlags = flags.shadow_matrices,
                         .pImmutableSamplers = nullptr});
        }

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

        if (flags.objects)
        {
                bindings.push_back(
                        {.binding = OBJECTS_BINDING,
                         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                         .descriptorCount = 1,
                         .stageFlags = flags.objects,
                         .pImmutableSamplers = nullptr});
        }

        if (flags.ggx_f1_albedo)
        {
                bindings.push_back(
                        {.binding = GGX_F1_ALBEDO_COSINE_ROUGHNESS_BINDING,
                         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                         .descriptorCount = 1,
                         .stageFlags = flags.ggx_f1_albedo,
                         .pImmutableSamplers = nullptr});

                bindings.push_back(
                        {.binding = GGX_F1_ALBEDO_COSINE_WEIGHTED_AVERAGE_BINDING,
                         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                         .descriptorCount = 1,
                         .stageFlags = flags.ggx_f1_albedo,
                         .pImmutableSamplers = nullptr});
        }

        if (flags.transparency)
        {
                bindings.push_back(
                        {.binding = TRANSPARENCY_HEADS_BINDING,
                         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                         .descriptorCount = 1,
                         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                         .pImmutableSamplers = nullptr});

                bindings.push_back(
                        {.binding = TRANSPARENCY_HEADS_SIZE_BINDING,
                         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                         .descriptorCount = 1,
                         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                         .pImmutableSamplers = nullptr});

                bindings.push_back(
                        {.binding = TRANSPARENCY_COUNTERS_BINDING,
                         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                         .descriptorCount = 1,
                         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                         .pImmutableSamplers = nullptr});

                bindings.push_back(
                        {.binding = TRANSPARENCY_NODES_BINDING,
                         .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                         .descriptorCount = 1,
                         .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                         .pImmutableSamplers = nullptr});
        }

        return bindings;
}

SharedMemory::SharedMemory(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const vulkan::Buffer& drawing)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings)
{
        descriptors_.update_descriptor_set(
                0, DRAWING_BINDING,
                VkDescriptorBufferInfo{.buffer = drawing.handle(), .offset = 0, .range = drawing.size()});
}

unsigned SharedMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& SharedMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

void SharedMemory::set_shadow_matrices(const vulkan::Buffer& shadow_matrices) const
{
        descriptors_.update_descriptor_set(
                0, SHADOW_MATRICES_BINDING,
                VkDescriptorBufferInfo{
                        .buffer = shadow_matrices.handle(),
                        .offset = 0,
                        .range = shadow_matrices.size()});
}

void SharedMemory::set_ggx_f1_albedo(
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

void SharedMemory::set_objects_image(const vulkan::ImageView& objects) const
{
        ASSERT(objects.format() == VK_FORMAT_R32_UINT);
        ASSERT(objects.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        descriptors_.update_descriptor_set(
                0, OBJECTS_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = objects.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});
}

void SharedMemory::set_transparency(
        const vulkan::ImageView& heads,
        const vulkan::ImageView& heads_size,
        const vulkan::Buffer& counters,
        const vulkan::Buffer& nodes) const
{
        static constexpr unsigned DESCRIPTOR_INDEX = 0;

        ASSERT(heads.format() == VK_FORMAT_R32_UINT);
        ASSERT(heads.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(heads_size.format() == VK_FORMAT_R32_UINT);
        ASSERT(heads_size.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(counters.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
        ASSERT(nodes.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(4);

        infos.emplace_back(
                DESCRIPTOR_INDEX, TRANSPARENCY_HEADS_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = heads.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

        infos.emplace_back(
                DESCRIPTOR_INDEX, TRANSPARENCY_HEADS_SIZE_BINDING,
                VkDescriptorImageInfo{
                        .sampler = VK_NULL_HANDLE,
                        .imageView = heads_size.handle(),
                        .imageLayout = VK_IMAGE_LAYOUT_GENERAL});

        infos.emplace_back(
                DESCRIPTOR_INDEX, TRANSPARENCY_COUNTERS_BINDING,
                VkDescriptorBufferInfo{.buffer = counters.handle(), .offset = 0, .range = counters.size()});

        infos.emplace_back(
                DESCRIPTOR_INDEX, TRANSPARENCY_NODES_BINDING,
                VkDescriptorBufferInfo{.buffer = nodes.handle(), .offset = 0, .range = nodes.size()});

        descriptors_.update_descriptor_set(infos);
}

void SharedMemory::set_shadow_image(const VkSampler sampler, const vulkan::ImageView& shadow_image) const
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

void SharedMemory::set_acceleration_structure(const VkAccelerationStructureKHR acceleration_structure) const
{
        descriptors_.update_descriptor_set(0, ACCELERATION_STRUCTURE_BINDING, acceleration_structure);
}

//

std::vector<VkDescriptorSetLayoutBinding> MeshMemory::descriptor_set_layout_bindings(
        const VkShaderStageFlags coordinates)
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(1);

        bindings.push_back(
                {.binding = BUFFER_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = coordinates | VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

MeshMemory::MeshMemory(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const vulkan::Buffer& buffer)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings)
{
        ASSERT(buffer.handle() != VK_NULL_HANDLE && buffer.size() > 0);

        descriptors_.update_descriptor_set(
                0, BUFFER_BINDING,
                VkDescriptorBufferInfo{.buffer = buffer.handle(), .offset = 0, .range = buffer.size()});
}

unsigned MeshMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& MeshMemory::descriptor_set() const
{
        return descriptors_.descriptor_set(0);
}

//

std::vector<VkDescriptorSetLayoutBinding> MaterialMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        bindings.reserve(2);

        bindings.push_back(
                {.binding = MATERIAL_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        bindings.push_back(
                {.binding = TEXTURE_BINDING,
                 .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                 .descriptorCount = 1,
                 .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                 .pImmutableSamplers = nullptr});

        return bindings;
}

MaterialMemory::MaterialMemory(
        const VkDevice device,
        const VkSampler sampler,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const std::vector<MaterialInfo>& materials)
        : descriptors_(device, materials.size(), descriptor_set_layout, descriptor_set_layout_bindings)
{
        ASSERT(!materials.empty());
        ASSERT(std::all_of(
                materials.cbegin(), materials.cend(),
                [](const MaterialInfo& m)
                {
                        return m.buffer != VK_NULL_HANDLE && m.buffer_size > 0 && m.texture != VK_NULL_HANDLE;
                }));

        std::vector<vulkan::Descriptors::DescriptorInfo> infos;
        infos.reserve(2 * materials.size());

        for (std::size_t index = 0; index < materials.size(); ++index)
        {
                const MaterialInfo& material = materials[index];

                infos.emplace_back(
                        index, MATERIAL_BINDING,
                        VkDescriptorBufferInfo{.buffer = material.buffer, .offset = 0, .range = material.buffer_size});

                infos.emplace_back(
                        index, TEXTURE_BINDING,
                        VkDescriptorImageInfo{
                                .sampler = sampler,
                                .imageView = material.texture,
                                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        }

        descriptors_.update_descriptor_set(infos);
}

unsigned MaterialMemory::set_number()
{
        return SET_NUMBER;
}

std::uint32_t MaterialMemory::descriptor_set_count() const
{
        return descriptors_.descriptor_set_count();
}

const VkDescriptorSet& MaterialMemory::descriptor_set(const std::uint32_t index) const
{
        return descriptors_.descriptor_set(index);
}
}
