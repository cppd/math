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
        std::vector<vulkan::Descriptors::Info> infos;
        std::vector<std::uint32_t> bindings;

        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = drawing.handle();
                buffer_info.offset = 0;
                buffer_info.range = drawing.size();

                infos.emplace_back(buffer_info);
                bindings.push_back(DRAWING_BINDING);
        }

        descriptors_.update_descriptor_set(0, bindings, infos);
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
        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = shadow_matrices.handle();
        buffer_info.offset = 0;
        buffer_info.range = shadow_matrices.size();

        descriptors_.update_descriptor_set(0, SHADOW_MATRICES_BINDING, buffer_info);
}

void SharedMemory::set_ggx_f1_albedo(
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

void SharedMemory::set_objects_image(const vulkan::ImageView& objects) const
{
        ASSERT(objects.format() == VK_FORMAT_R32_UINT);
        ASSERT(objects.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = objects.handle();

        descriptors_.update_descriptor_set(0, OBJECTS_BINDING, image_info);
}

void SharedMemory::set_transparency(
        const vulkan::ImageView& heads,
        const vulkan::ImageView& heads_size,
        const vulkan::Buffer& counters,
        const vulkan::Buffer& nodes) const
{
        ASSERT(heads.format() == VK_FORMAT_R32_UINT);
        ASSERT(heads.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(heads_size.format() == VK_FORMAT_R32_UINT);
        ASSERT(heads_size.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(counters.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
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
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                image_info.imageView = heads_size.handle();

                infos.emplace_back(image_info);
                bindings.push_back(TRANSPARENCY_HEADS_SIZE_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = counters.handle();
                buffer_info.offset = 0;
                buffer_info.range = counters.size();

                infos.emplace_back(buffer_info);
                bindings.push_back(TRANSPARENCY_COUNTERS_BINDING);
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

void SharedMemory::set_shadow_image(const VkSampler sampler, const vulkan::ImageView& shadow_image) const
{
        ASSERT(shadow_image.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));
        ASSERT(shadow_image.sample_count() == VK_SAMPLE_COUNT_1_BIT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = shadow_image.handle();
        image_info.sampler = sampler;

        descriptors_.update_descriptor_set(0, SHADOW_MAP_BINDING, image_info);
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

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = buffer.handle();
        buffer_info.offset = 0;
        buffer_info.range = buffer.size();

        descriptors_.update_descriptor_set(0, BUFFER_BINDING, buffer_info);
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

        std::vector<vulkan::Descriptors::Info> infos;
        std::vector<std::uint32_t> bindings;

        for (std::size_t i = 0; i < materials.size(); ++i)
        {
                const MaterialInfo& material = materials[i];

                infos.clear();
                bindings.clear();
                {
                        VkDescriptorBufferInfo buffer_info = {};
                        buffer_info.buffer = material.buffer;
                        buffer_info.offset = 0;
                        buffer_info.range = material.buffer_size;

                        infos.emplace_back(buffer_info);
                        bindings.push_back(MATERIAL_BINDING);
                }
                {
                        VkDescriptorImageInfo image_info = {};
                        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        image_info.imageView = material.texture;
                        image_info.sampler = sampler;

                        infos.emplace_back(image_info);
                        bindings.push_back(TEXTURE_BINDING);
                }
                descriptors_.update_descriptor_set(i, bindings, infos);
        }
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
