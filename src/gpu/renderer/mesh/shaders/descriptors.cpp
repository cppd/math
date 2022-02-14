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
SharedConstants::SharedConstants()
{
        VkSpecializationMapEntry entry = {};
        entry.constantID = 0;
        entry.offset = offsetof(Data, transparency_drawing);
        entry.size = sizeof(Data::transparency_drawing);
        entries_.push_back(entry);
}

void SharedConstants::set(const bool transparency_drawing)
{
        data_.transparency_drawing = transparency_drawing ? 1 : 0;
}

const std::vector<VkSpecializationMapEntry>& SharedConstants::entries() const
{
        return entries_;
}

const void* SharedConstants::data() const
{
        return &data_;
}

std::size_t SharedConstants::size() const
{
        return sizeof(data_);
}

//

std::vector<VkDescriptorSetLayoutBinding> SharedMemory::descriptor_set_layout_bindings(
        const VkShaderStageFlags shadow_matrices,
        const VkShaderStageFlags drawing,
        const VkShaderStageFlags objects,
        const VkShaderStageFlags shadow_map,
        const VkShaderStageFlags acceleration_structure,
        const VkShaderStageFlags ggx_f1_albedo)
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DRAWING_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = drawing | VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        if (shadow_matrices)
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = SHADOW_MATRICES_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = shadow_matrices;

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
        if (objects)
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OBJECTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = objects;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        if (ggx_f1_albedo)
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = GGX_F1_ALBEDO_COSINE_ROUGHNESS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = ggx_f1_albedo;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        if (ggx_f1_albedo)
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = GGX_F1_ALBEDO_COSINE_WEIGHTED_AVERAGE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = ggx_f1_albedo;
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
                b.binding = TRANSPARENCY_HEADS_SIZE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TRANSPARENCY_COUNTERS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

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

        return bindings;
}

SharedMemory::SharedMemory(
        const vulkan::Device& device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const vulkan::Buffer& drawing)
        : descriptors_(device, 1, descriptor_set_layout, descriptor_set_layout_bindings)
{
        std::vector<vulkan::Descriptors::Info> infos;
        std::vector<std::uint32_t> bindings;

        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = drawing;
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
        buffer_info.buffer = shadow_matrices;
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

void SharedMemory::set_objects_image(const vulkan::ImageView& objects) const
{
        ASSERT(objects.format() == VK_FORMAT_R32_UINT);
        ASSERT(objects.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = objects;

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
                image_info.imageView = heads;

                infos.emplace_back(image_info);
                bindings.push_back(TRANSPARENCY_HEADS_BINDING);
        }
        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                image_info.imageView = heads_size;

                infos.emplace_back(image_info);
                bindings.push_back(TRANSPARENCY_HEADS_SIZE_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = counters;
                buffer_info.offset = 0;
                buffer_info.range = counters.size();

                infos.emplace_back(buffer_info);
                bindings.push_back(TRANSPARENCY_COUNTERS_BINDING);
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

void SharedMemory::set_shadow_image(const VkSampler sampler, const vulkan::ImageView& shadow_image) const
{
        ASSERT(shadow_image.has_usage(VK_IMAGE_USAGE_SAMPLED_BIT));
        ASSERT(shadow_image.sample_count() == VK_SAMPLE_COUNT_1_BIT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = shadow_image;
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

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

                b.descriptorCount = 1;
                b.stageFlags = coordinates | VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

vulkan::Descriptors MeshMemory::create(
        const VkDevice device,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const std::vector<const vulkan::Buffer*>& coordinates)
{
        ASSERT(!coordinates.empty());
        ASSERT(std::all_of(
                coordinates.cbegin(), coordinates.cend(),
                [](const vulkan::Buffer* const buffer)
                {
                        return buffer != nullptr;
                }));

        vulkan::Descriptors descriptors(
                vulkan::Descriptors(device, coordinates.size(), descriptor_set_layout, descriptor_set_layout_bindings));

        std::vector<vulkan::Descriptors::Info> infos;
        std::vector<std::uint32_t> bindings;

        for (std::size_t i = 0; i < coordinates.size(); ++i)
        {
                infos.clear();
                bindings.clear();
                {
                        VkDescriptorBufferInfo buffer_info = {};
                        buffer_info.buffer = *coordinates[i];
                        buffer_info.offset = 0;
                        buffer_info.range = coordinates[i]->size();

                        infos.emplace_back(buffer_info);
                        bindings.push_back(BUFFER_BINDING);
                }
                descriptors.update_descriptor_set(i, bindings, infos);
        }

        return descriptors;
}

unsigned MeshMemory::set_number()
{
        return SET_NUMBER;
}

//

std::vector<VkDescriptorSetLayoutBinding> MaterialMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = MATERIAL_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TEXTURE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

vulkan::Descriptors MaterialMemory::create(
        const VkDevice device,
        const VkSampler sampler,
        const VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const std::vector<MaterialInfo>& materials)
{
        ASSERT(!materials.empty());
        ASSERT(std::all_of(
                materials.cbegin(), materials.cend(),
                [](const MaterialInfo& m)
                {
                        return m.buffer != VK_NULL_HANDLE && m.buffer_size > 0 && m.texture != VK_NULL_HANDLE;
                }));

        vulkan::Descriptors descriptors(
                vulkan::Descriptors(device, materials.size(), descriptor_set_layout, descriptor_set_layout_bindings));

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
                descriptors.update_descriptor_set(i, bindings, infos);
        }

        return descriptors;
}

unsigned MaterialMemory::set_number()
{
        return SET_NUMBER;
}
}
