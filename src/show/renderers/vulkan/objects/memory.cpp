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

#include "memory.h"

#include "com/error.h"

namespace vulkan_renderer_implementation
{
std::vector<VkDescriptorSetLayoutBinding> TrianglesSharedMemory::descriptor_set_layout_bindings()
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
                b.binding = LIGHTING_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

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
                b.binding = SHADOW_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OBJECTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

TrianglesSharedMemory::TrianglesSharedMemory(const vulkan::Device& device)
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
                m_uniform_buffers.emplace_back(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Lighting));
                m_lighting_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.push_back(buffer_info);

                bindings.push_back(LIGHTING_BINDING);
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

VkDescriptorSetLayout TrianglesSharedMemory::descriptor_set_layout() const noexcept
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& TrianglesSharedMemory::descriptor_set() const noexcept
{
        return m_descriptors.descriptor_set(0);
}

template <typename T>
void TrianglesSharedMemory::copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const
{
        m_uniform_buffers[m_matrices_buffer_index].write(offset, data);
}
template <typename T>
void TrianglesSharedMemory::copy_to_lighting_buffer(VkDeviceSize offset, const T& data) const
{
        m_uniform_buffers[m_lighting_buffer_index].write(offset, data);
}
template <typename T>
void TrianglesSharedMemory::copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const
{
        m_uniform_buffers[m_drawing_buffer_index].write(offset, data);
}

void TrianglesSharedMemory::set_matrices(const mat4& matrix, const mat4& shadow_matrix) const
{
        Matrices matrices;
        matrices.matrix = transpose(to_matrix<float>(matrix));
        matrices.shadow_matrix = transpose(to_matrix<float>(shadow_matrix));
        copy_to_matrices_buffer(0, matrices);
}

void TrianglesSharedMemory::set_default_color(const Color& color) const
{
        decltype(Drawing().default_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, default_color), c);
}
void TrianglesSharedMemory::set_wireframe_color(const Color& color) const
{
        decltype(Drawing().wireframe_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, wireframe_color), c);
}
void TrianglesSharedMemory::set_default_ns(float default_ns) const
{
        decltype(Drawing().default_ns) ns = default_ns;
        copy_to_drawing_buffer(offsetof(Drawing, default_ns), ns);
}
void TrianglesSharedMemory::set_light_a(const Color& color) const
{
        decltype(Drawing().light_a) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_a), c);
}
void TrianglesSharedMemory::set_light_d(const Color& color) const
{
        decltype(Drawing().light_d) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_d), c);
}
void TrianglesSharedMemory::set_light_s(const Color& color) const
{
        decltype(Drawing().light_s) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_s), c);
}
void TrianglesSharedMemory::set_show_materials(bool show) const
{
        decltype(Drawing().show_materials) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_materials), s);
}
void TrianglesSharedMemory::set_show_wireframe(bool show) const
{
        decltype(Drawing().show_wireframe) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_wireframe), s);
}
void TrianglesSharedMemory::set_show_shadow(bool show) const
{
        decltype(Drawing().show_shadow) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_shadow), s);
}

void TrianglesSharedMemory::set_direction_to_light(const vec3f& direction) const
{
        decltype(Lighting().direction_to_light) d = direction;
        copy_to_lighting_buffer(offsetof(Lighting, direction_to_light), d);
}
void TrianglesSharedMemory::set_direction_to_camera(const vec3f& direction) const
{
        decltype(Lighting().direction_to_camera) d = direction;
        copy_to_lighting_buffer(offsetof(Lighting, direction_to_camera), d);
}
void TrianglesSharedMemory::set_show_smooth(bool show) const
{
        decltype(Lighting().show_smooth) s = show ? 1 : 0;
        copy_to_lighting_buffer(offsetof(Lighting, show_smooth), s);
}
void TrianglesSharedMemory::set_shadow_texture(VkSampler sampler, const vulkan::ShadowDepthAttachment* shadow_texture) const
{
        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = shadow_texture->image_layout();
        image_info.imageView = shadow_texture->image_view();
        image_info.sampler = sampler;

        m_descriptors.update_descriptor_set(0, SHADOW_BINDING, image_info);
}
void TrianglesSharedMemory::set_object_image(const vulkan::StorageImage* storage_image) const
{
        ASSERT(storage_image && storage_image->format() == VK_FORMAT_R32_UINT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = storage_image->image_layout();
        image_info.imageView = storage_image->image_view();

        m_descriptors.update_descriptor_set(0, OBJECTS_BINDING, image_info);
}

//

std::vector<VkDescriptorSetLayoutBinding> TrianglesMaterialMemory::descriptor_set_layout_bindings()
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
                b.binding = TEXTURE_KA_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TEXTURE_KD_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = TEXTURE_KS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

TrianglesMaterialMemory::TrianglesMaterialMemory(const vulkan::Device& device, VkSampler sampler,
                                                 VkDescriptorSetLayout descriptor_set_layout,
                                                 const std::vector<MaterialAndTexture>& materials)
        : m_descriptors(vulkan::Descriptors(device, materials.size(), descriptor_set_layout, descriptor_set_layout_bindings()))
{
        ASSERT(materials.size() > 0);
        ASSERT(std::all_of(materials.cbegin(), materials.cend(),
                           [](const MaterialAndTexture& m) { return m.texture_Ka && m.texture_Kd && m.texture_Ks; }));

        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        for (size_t i = 0; i < materials.size(); ++i)
        {
                const MaterialAndTexture& material = materials[i];

                infos.clear();
                bindings.clear();
                {
                        m_uniform_buffers.emplace_back(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Material));

                        VkDescriptorBufferInfo buffer_info = {};
                        buffer_info.buffer = m_uniform_buffers.back();
                        buffer_info.offset = 0;
                        buffer_info.range = m_uniform_buffers.back().size();

                        infos.push_back(buffer_info);

                        bindings.push_back(MATERIAL_BINDING);
                }
                {
                        VkDescriptorImageInfo image_info = {};
                        image_info.imageLayout = material.texture_Ka->image_layout();
                        image_info.imageView = material.texture_Ka->image_view();
                        image_info.sampler = sampler;

                        infos.push_back(image_info);

                        bindings.push_back(TEXTURE_KA_BINDING);
                }
                {
                        VkDescriptorImageInfo image_info = {};
                        image_info.imageLayout = material.texture_Kd->image_layout();
                        image_info.imageView = material.texture_Kd->image_view();
                        image_info.sampler = sampler;

                        infos.push_back(image_info);

                        bindings.push_back(TEXTURE_KD_BINDING);
                }
                {
                        VkDescriptorImageInfo image_info = {};
                        image_info.imageLayout = material.texture_Ks->image_layout();
                        image_info.imageView = material.texture_Ks->image_view();
                        image_info.sampler = sampler;

                        infos.push_back(image_info);

                        bindings.push_back(TEXTURE_KS_BINDING);
                }
                m_descriptors.update_descriptor_set(i, bindings, infos);
        }

        for (size_t i = 0; i < materials.size(); ++i)
        {
                m_uniform_buffers[i].write(materials[i].material);
        }
}

uint32_t TrianglesMaterialMemory::descriptor_set_count() const noexcept
{
        return m_descriptors.descriptor_set_count();
}

const VkDescriptorSet& TrianglesMaterialMemory::descriptor_set(uint32_t index) const noexcept
{
        return m_descriptors.descriptor_set(index);
}

//

std::vector<VkDescriptorSetLayoutBinding> ShadowMemory::descriptor_set_layout_bindings()
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

        return bindings;
}

ShadowMemory::ShadowMemory(const vulkan::Device& device)
        : m_descriptor_set_layout(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          m_descriptors(device, 1, m_descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                m_uniform_buffers.emplace_back(device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Matrices));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.push_back(buffer_info);

                bindings.push_back(MATRICES_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

VkDescriptorSetLayout ShadowMemory::descriptor_set_layout() const noexcept
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& ShadowMemory::descriptor_set() const noexcept
{
        return m_descriptors.descriptor_set(0);
}

void ShadowMemory::set_matrix(const mat4& matrix) const
{
        decltype(Matrices().matrix) m = transpose(to_matrix<float>(matrix));
        m_uniform_buffers[0].write(offsetof(Matrices, matrix), m);
}

//

std::vector<VkDescriptorSetLayoutBinding> PointsMemory::descriptor_set_layout_bindings()
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
                b.binding = DRAWING_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

                bindings.push_back(b);
        }
        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = OBJECTS_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                b.pImmutableSamplers = nullptr;

                bindings.push_back(b);
        }

        return bindings;
}

PointsMemory::PointsMemory(const vulkan::Device& device)
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

VkDescriptorSetLayout PointsMemory::descriptor_set_layout() const noexcept
{
        return m_descriptor_set_layout;
}

const VkDescriptorSet& PointsMemory::descriptor_set() const noexcept
{
        return m_descriptors.descriptor_set(0);
}

template <typename T>
void PointsMemory::copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const
{
        m_uniform_buffers[m_matrices_buffer_index].write(offset, data);
}
template <typename T>
void PointsMemory::copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const
{
        m_uniform_buffers[m_drawing_buffer_index].write(offset, data);
}

void PointsMemory::set_matrix(const mat4& matrix) const
{
        decltype(Matrices().matrix) m = transpose(to_matrix<float>(matrix));
        copy_to_matrices_buffer(offsetof(Matrices, matrix), m);
}

void PointsMemory::set_default_color(const Color& color) const
{
        decltype(Drawing().default_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, default_color), c);
}
void PointsMemory::set_background_color(const Color& color) const
{
        decltype(Drawing().background_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, background_color), c);
}
void PointsMemory::set_light_a(const Color& color) const
{
        decltype(Drawing().light_a) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_a), c);
}
void PointsMemory::set_show_fog(bool show) const
{
        decltype(Drawing().show_fog) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_fog), s);
}
void PointsMemory::set_object_image(const vulkan::StorageImage* storage_image) const
{
        ASSERT(storage_image && storage_image->format() == VK_FORMAT_R32_UINT);

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = storage_image->image_layout();
        image_info.imageView = storage_image->image_view();

        m_descriptors.update_descriptor_set(0, OBJECTS_BINDING, image_info);
}
}
