/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "vulkan_memory.h"

namespace vulkan_text_implementation
{
std::vector<VkDescriptorSetLayoutBinding> TextMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = MATRICES_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TEXTURE_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

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

        return bindings;
}

TextMemory::TextMemory(const vulkan::Device& device, VkSampler sampler, const vulkan::GrayscaleTexture* texture)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                m_uniform_buffers.emplace_back(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Matrices));
                m_matrices_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.push_back(buffer_info);

                bindings.push_back(MATRICES_BINDING);
        }
        {
                VkDescriptorImageInfo image_info = {};
                image_info.imageLayout = texture->image_layout();
                image_info.imageView = texture->image_view();
                image_info.sampler = sampler;

                infos.push_back(image_info);

                bindings.push_back(TEXTURE_BINDING);
        }
        {
                m_uniform_buffers.emplace_back(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Drawing));
                m_drawing_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.push_back(buffer_info);

                bindings.push_back(DRAWING_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

unsigned TextMemory::set_number() noexcept
{
        return SET_NUMBER;
}

VkDescriptorSetLayout TextMemory::descriptor_set_layout() const noexcept
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& TextMemory::descriptor_set() const noexcept
{
        return m_descriptors.descriptor_set(0);
}

template <typename T>
void TextMemory::copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const
{
        m_uniform_buffers[m_matrices_buffer_index].write(offset, data);
}
template <typename T>
void TextMemory::copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const
{
        m_uniform_buffers[m_drawing_buffer_index].write(offset, data);
}

void TextMemory::set_matrix(const mat4& matrix) const
{
        decltype(Matrices().matrix) m = transpose(to_matrix<float>(matrix));
        copy_to_matrices_buffer(offsetof(Matrices, matrix), m);
}

void TextMemory::set_color(const Color& color) const
{
        decltype(Drawing().color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, color), c);
}
}
