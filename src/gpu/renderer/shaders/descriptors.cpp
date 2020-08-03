/*
Copyright (C) 2017-2020 Topological Manifold

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

namespace gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> CommonMemory::descriptor_set_layout_bindings(
        VkShaderStageFlags matrices,
        VkShaderStageFlags drawing,
        VkShaderStageFlags shadow,
        VkShaderStageFlags objects)
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = MATRICES_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                if (matrices)
                {
                        b.descriptorCount = 1;
                        b.stageFlags = matrices;
                }

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = DRAWING_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                if (drawing)
                {
                        b.descriptorCount = 1;
                        b.stageFlags = drawing;
                }

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = SHADOW_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                if (shadow)
                {
                        b.descriptorCount = 1;
                        b.stageFlags = shadow;
                }
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OBJECTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                if (objects)
                {
                        b.descriptorCount = 1;
                        b.stageFlags = objects;
                }
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
                b.binding = TRANSPARENCY_COUNTER_BINDING;
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

CommonMemory::CommonMemory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const vulkan::Buffer& matrices,
        const vulkan::Buffer& drawing)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings)
{
        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = matrices;
                buffer_info.offset = 0;
                buffer_info.range = matrices.size();

                infos.emplace_back(buffer_info);

                bindings.push_back(MATRICES_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = drawing;
                buffer_info.offset = 0;
                buffer_info.range = drawing.size();

                infos.emplace_back(buffer_info);

                bindings.push_back(DRAWING_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

unsigned CommonMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& CommonMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void CommonMemory::set_shadow_texture(VkSampler sampler, const vulkan::DepthImageWithMemory* shadow_texture) const
{
        ASSERT(shadow_texture && (shadow_texture->usage() & VK_IMAGE_USAGE_SAMPLED_BIT));
        ASSERT(shadow_texture && (shadow_texture->sample_count() == VK_SAMPLE_COUNT_1_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = shadow_texture->image_view();
        image_info.sampler = sampler;

        m_descriptors.update_descriptor_set(0, SHADOW_BINDING, image_info);
}

void CommonMemory::set_objects_image(const vulkan::ImageWithMemory& objects) const
{
        ASSERT(objects.format() == VK_FORMAT_R32_UINT);
        ASSERT(objects.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = objects.image_view();

        m_descriptors.update_descriptor_set(0, OBJECTS_BINDING, image_info);
}

void CommonMemory::set_transparency(
        const vulkan::ImageWithMemory& heads,
        const vulkan::Buffer& counter,
        const vulkan::Buffer& nodes) const
{
        ASSERT(heads.format() == VK_FORMAT_R32_UINT);
        ASSERT(heads.has_usage(VK_IMAGE_USAGE_STORAGE_BIT));
        ASSERT(counter.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
        ASSERT(nodes.has_usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));

        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                image_info.imageView = heads.image_view();

                infos.emplace_back(image_info);
                bindings.push_back(TRANSPARENCY_HEADS_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = counter;
                buffer_info.offset = 0;
                buffer_info.range = counter.size();

                infos.emplace_back(buffer_info);

                bindings.push_back(TRANSPARENCY_COUNTER_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = nodes;
                buffer_info.offset = 0;
                buffer_info.range = nodes.size();

                infos.emplace_back(buffer_info);

                bindings.push_back(TRANSPARENCY_NODES_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

//

std::vector<VkDescriptorSetLayoutBinding> MeshMemory::descriptor_set_layout_bindings(VkShaderStageFlags coordinates)
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                if (coordinates)
                {
                        b.descriptorCount = 1;
                        b.stageFlags = coordinates;
                }

                bindings.push_back(b);
        }

        return bindings;
}

vulkan::Descriptors MeshMemory::create(
        VkDevice device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<VkDescriptorSetLayoutBinding>& descriptor_set_layout_bindings,
        const std::vector<const vulkan::Buffer*>& coordinates)
{
        ASSERT(!coordinates.empty());
        ASSERT(std::all_of(coordinates.cbegin(), coordinates.cend(), [](const vulkan::Buffer* buffer) {
                return buffer != nullptr;
        }));

        vulkan::Descriptors descriptors(
                vulkan::Descriptors(device, coordinates.size(), descriptor_set_layout, descriptor_set_layout_bindings));

        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        for (size_t i = 0; i < coordinates.size(); ++i)
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
}
