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
void copy_to_buffer(const vulkan::UniformBufferWithHostVisibleMemory& buffer, VkDeviceSize offset, const T& data)
{
        buffer.copy(offset, &data, sizeof(data));
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
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(Vertex, geometric_normal);

                descriptions.push_back(d);
        }
        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 3;
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
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 1;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 2;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

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
                m_matrices_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.push_back(buffer_info);
        }
        {
                m_uniform_buffers.emplace_back(device, sizeof(Lighting));
                m_lighting_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.push_back(buffer_info);
        }
        {
                m_uniform_buffers.emplace_back(device, sizeof(Drawing));
                m_drawing_buffer_index = m_uniform_buffers.size() - 1;

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

template <typename T>
void SharedMemory::copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const
{
        copy_to_buffer(m_uniform_buffers[m_matrices_buffer_index], offset, data);
}
template <typename T>
void SharedMemory::copy_to_lighting_buffer(VkDeviceSize offset, const T& data) const
{
        copy_to_buffer(m_uniform_buffers[m_lighting_buffer_index], offset, data);
}
template <typename T>
void SharedMemory::copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const
{
        copy_to_buffer(m_uniform_buffers[m_drawing_buffer_index], offset, data);
}

void SharedMemory::set_matrix(const mat4& matrix) const
{
        decltype(Matrices().mvp) m = transpose(to_matrix<float>(matrix));
        copy_to_matrices_buffer(offsetof(Matrices, mvp), m);
}
void SharedMemory::set_default_color(const Color& color) const
{
        decltype(Drawing().default_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, default_color), c);
}
void SharedMemory::set_wireframe_color(const Color& color) const
{
        decltype(Drawing().wireframe_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, wireframe_color), c);
}
void SharedMemory::set_default_ns(float default_ns) const
{
        decltype(Drawing().default_ns) ns = default_ns;
        copy_to_drawing_buffer(offsetof(Drawing, default_ns), ns);
}
void SharedMemory::set_light_a(const Color& color) const
{
        decltype(Drawing().light_a) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_a), c);
}
void SharedMemory::set_light_d(const Color& color) const
{
        decltype(Drawing().light_d) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_d), c);
}
void SharedMemory::set_light_s(const Color& color) const
{
        decltype(Drawing().light_s) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_s), c);
}
void SharedMemory::set_show_materials(bool show) const
{
        decltype(Drawing().show_materials) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_materials), s);
}
void SharedMemory::set_direction_to_light(const vec3f& direction) const
{
        decltype(Lighting().direction_to_light) d = direction;
        copy_to_lighting_buffer(offsetof(Lighting, direction_to_light), d);
}
void SharedMemory::set_direction_to_camera(const vec3f& direction) const
{
        decltype(Lighting().direction_to_camera) d = direction;
        copy_to_lighting_buffer(offsetof(Lighting, direction_to_camera), d);
}
void SharedMemory::set_show_smooth(bool show) const
{
        decltype(Lighting().show_smooth) s = show ? 1 : 0;
        copy_to_lighting_buffer(offsetof(Lighting, show_smooth), s);
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
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 2;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = 3;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

PerObjectMemory::PerObjectMemory(const vulkan::Device& device, VkSampler sampler, VkDescriptorSetLayout descriptor_set_layout,
                                 const std::vector<MaterialAndTexture>& materials)
        : m_descriptors(vulkan::Descriptors(device, materials.size(), descriptor_set_layout, descriptor_set_layout_bindings()))
{
        ASSERT(materials.size() > 0);
        ASSERT(std::all_of(materials.cbegin(), materials.cend(),
                           [](const MaterialAndTexture& m) { return m.texture_Ka && m.texture_Kd && m.texture_Ks; }));

        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;

        for (const MaterialAndTexture& material : materials)
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
                        image_info.imageLayout = material.texture_Ka->image_layout();
                        image_info.imageView = material.texture_Ka->image_view();
                        image_info.sampler = sampler;

                        infos.push_back(image_info);
                }
                {
                        VkDescriptorImageInfo image_info = {};
                        image_info.imageLayout = material.texture_Kd->image_layout();
                        image_info.imageView = material.texture_Kd->image_view();
                        image_info.sampler = sampler;

                        infos.push_back(image_info);
                }
                {
                        VkDescriptorImageInfo image_info = {};
                        image_info.imageLayout = material.texture_Ks->image_layout();
                        image_info.imageView = material.texture_Ks->image_view();
                        image_info.sampler = sampler;

                        infos.push_back(image_info);
                }
                m_descriptor_sets.push_back(m_descriptors.create_descriptor_set(infos));
        }

        ASSERT(m_descriptor_sets.size() == materials.size());

        for (size_t i = 0; i < materials.size(); ++i)
        {
                copy_to_buffer(m_uniform_buffers[i], 0 /*offset*/, materials[i].material);
        }
}

uint32_t PerObjectMemory::descriptor_set_count() const noexcept
{
        return m_descriptor_sets.size();
}

VkDescriptorSet PerObjectMemory::descriptor_set(uint32_t index) const noexcept
{
        ASSERT(index < m_descriptor_sets.size());

        return m_descriptor_sets[index];
}
}
