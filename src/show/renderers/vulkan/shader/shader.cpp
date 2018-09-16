/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "shader.h"

#include "com/error.h"

namespace
{
template <typename T>
void copy_to_buffer(const std::vector<vulkan::UniformBufferWithHostVisibleMemory>& buffers, uint32_t index, const T& data)
{
        ASSERT(index < buffers.size());
        ASSERT(sizeof(data) == buffers[index].size());

        buffers[index].copy(&data);
}

template <typename T>
void copy_to_buffer(const vulkan::UniformBufferWithHostVisibleMemory& buffer, const T& data)
{
        ASSERT(sizeof(data) == buffer.size());

        buffer.copy(&data);
}
}

namespace vulkan_renderer_shaders
{
std::vector<VkVertexInputBindingDescription> Vertex::binding_descriptions()
{
        std::vector<VkVertexInputBindingDescription> descriptions;

        {
                VkVertexInputBindingDescription d = {};
                d.binding = 0;
                d.stride = sizeof(Vertex);
                d.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                descriptions.push_back(d);
        }

        return descriptions;
}

std::vector<VkVertexInputAttributeDescription> Vertex::attribute_descriptions()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;

        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 0;
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(Vertex, position);

                descriptions.push_back(d);
        }
        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 1;
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(Vertex, normal);

                descriptions.push_back(d);
        }
        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 2;
                d.format = VK_FORMAT_R32G32_SFLOAT;
                d.offset = offsetof(Vertex, texture_coordinates);

                descriptions.push_back(d);
        }

        return descriptions;
}

//

std::vector<VkDescriptorSetLayoutBinding> SharedMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 0;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

SharedMemory::SharedMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout)
        : m_descriptors(vulkan::Descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings()))
{
        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;

        {
                m_uniform_buffers.emplace_back(device, sizeof(Matrices));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.push_back(buffer_info);
        }

        m_descriptor_set = m_descriptors.create_descriptor_set(infos);
}

VkDescriptorSet SharedMemory::descriptor_set() const noexcept
{
        return m_descriptor_set;
}

void SharedMemory::set_uniform(const Matrices& matrices) const
{
        copy_to_buffer(m_uniform_buffers, 0, matrices);
}

//

std::vector<VkDescriptorSetLayoutBinding> PerObjectMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 0;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 1;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

PerObjectMemory::PerObjectMemory(const vulkan::Device& device, VkSampler sampler, VkDescriptorSetLayout descriptor_set_layout,
                                 const std::vector<Material>& materials, const std::vector<const vulkan::Texture*>& textures)
        : m_descriptors(vulkan::Descriptors(device, materials.size(), descriptor_set_layout, descriptor_set_layout_bindings()))
{
        ASSERT(materials.size() > 0);
        ASSERT(materials.size() == textures.size());
        ASSERT(std::all_of(textures.cbegin(), textures.cend(), [](const vulkan::Texture* t) { return t != nullptr; }));

        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;

        for (size_t i = 0; i < materials.size(); ++i)
        {
                infos.clear();
                {
                        m_uniform_buffers.emplace_back(device, sizeof(Material));

                        VkDescriptorBufferInfo buffer_info = {};
                        buffer_info.buffer = m_uniform_buffers.back();
                        buffer_info.offset = 0;
                        buffer_info.range = m_uniform_buffers.back().size();

                        infos.push_back(buffer_info);
                }
                {
                        VkDescriptorImageInfo image_info = {};
                        image_info.imageLayout = textures[i]->image_layout();
                        image_info.imageView = textures[i]->image_view();
                        image_info.sampler = sampler;

                        infos.push_back(image_info);
                }
                m_descriptor_sets.push_back(m_descriptors.create_descriptor_set(infos));
        }

        ASSERT(m_descriptor_sets.size() == materials.size());

        for (size_t i = 0; i < materials.size(); ++i)
        {
                copy_to_buffer(m_uniform_buffers[i], materials[i]);
        }
}

VkDescriptorSet PerObjectMemory::descriptor_set(uint32_t index) const noexcept
{
        ASSERT(index < m_descriptor_sets.size());

        return m_descriptor_sets[index];
}
}
