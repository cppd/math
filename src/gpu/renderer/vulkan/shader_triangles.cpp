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

#include "shader_triangles.h"

#include "shader_source.h"

#include <src/com/error.h>
#include <src/graphics/vulkan/create.h>
#include <src/graphics/vulkan/pipeline.h>

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> RendererTrianglesSharedMemory::descriptor_set_layout_bindings()
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

RendererTrianglesSharedMemory::RendererTrianglesSharedMemory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::unordered_set<uint32_t>& family_indices)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                m_uniform_buffers.emplace_back(
                        vulkan::BufferMemoryType::HostVisible, device, family_indices,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Matrices));
                m_matrices_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.emplace_back(buffer_info);

                bindings.push_back(MATRICES_BINDING);
        }
        {
                m_uniform_buffers.emplace_back(
                        vulkan::BufferMemoryType::HostVisible, device, family_indices,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Lighting));
                m_lighting_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.emplace_back(buffer_info);

                bindings.push_back(LIGHTING_BINDING);
        }
        {
                m_uniform_buffers.emplace_back(
                        vulkan::BufferMemoryType::HostVisible, device, family_indices,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Drawing));
                m_drawing_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.emplace_back(buffer_info);

                bindings.push_back(DRAWING_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

unsigned RendererTrianglesSharedMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& RendererTrianglesSharedMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

template <typename T>
void RendererTrianglesSharedMemory::copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_matrices_buffer_index], offset, data);
}
template <typename T>
void RendererTrianglesSharedMemory::copy_to_lighting_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_lighting_buffer_index], offset, data);
}
template <typename T>
void RendererTrianglesSharedMemory::copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_drawing_buffer_index], offset, data);
}

void RendererTrianglesSharedMemory::set_matrices(const mat4& main_mvp_matrix, const mat4& shadow_mvp_texture_matrix)
        const
{
        Matrices::M matrices;
        matrices.main_mvp_matrix = to_matrix<float>(main_mvp_matrix).transpose();
        matrices.shadow_mvp_texture_matrix = to_matrix<float>(shadow_mvp_texture_matrix).transpose();
        copy_to_matrices_buffer(offsetof(Matrices, matrices), matrices);
}

void RendererTrianglesSharedMemory::set_clip_plane(const vec4& equation, bool enabled) const
{
        Matrices::C clip_plane;
        clip_plane.equation = to_vector<float>(equation);
        clip_plane.enabled = enabled ? 1 : 0;
        copy_to_matrices_buffer(offsetof(Matrices, clip_plane), clip_plane);
}

void RendererTrianglesSharedMemory::set_default_color(const Color& color) const
{
        decltype(Drawing().default_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, default_color), c);
}
void RendererTrianglesSharedMemory::set_wireframe_color(const Color& color) const
{
        decltype(Drawing().wireframe_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, wireframe_color), c);
}
void RendererTrianglesSharedMemory::set_default_ns(float default_ns) const
{
        decltype(Drawing().default_ns) ns = default_ns;
        copy_to_drawing_buffer(offsetof(Drawing, default_ns), ns);
}
void RendererTrianglesSharedMemory::set_light_a(const Color& color) const
{
        decltype(Drawing().light_a) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_a), c);
}
void RendererTrianglesSharedMemory::set_light_d(const Color& color) const
{
        decltype(Drawing().light_d) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_d), c);
}
void RendererTrianglesSharedMemory::set_light_s(const Color& color) const
{
        decltype(Drawing().light_s) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_s), c);
}
void RendererTrianglesSharedMemory::set_show_materials(bool show) const
{
        decltype(Drawing().show_materials) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_materials), s);
}
void RendererTrianglesSharedMemory::set_show_wireframe(bool show) const
{
        decltype(Drawing().show_wireframe) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_wireframe), s);
}
void RendererTrianglesSharedMemory::set_show_shadow(bool show) const
{
        decltype(Drawing().show_shadow) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_shadow), s);
}

void RendererTrianglesSharedMemory::set_direction_to_light(const vec3f& direction) const
{
        decltype(Lighting().direction_to_light) d = direction;
        copy_to_lighting_buffer(offsetof(Lighting, direction_to_light), d);
}
void RendererTrianglesSharedMemory::set_direction_to_camera(const vec3f& direction) const
{
        decltype(Lighting().direction_to_camera) d = direction;
        copy_to_lighting_buffer(offsetof(Lighting, direction_to_camera), d);
}
void RendererTrianglesSharedMemory::set_show_smooth(bool show) const
{
        decltype(Lighting().show_smooth) s = show ? 1 : 0;
        copy_to_lighting_buffer(offsetof(Lighting, show_smooth), s);
}
void RendererTrianglesSharedMemory::set_shadow_texture(VkSampler sampler, const vulkan::DepthAttachment* shadow_texture)
        const
{
        ASSERT(shadow_texture && (shadow_texture->usage() & VK_IMAGE_USAGE_SAMPLED_BIT));
        ASSERT(shadow_texture && (shadow_texture->sample_count() == VK_SAMPLE_COUNT_1_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = shadow_texture->image_view();
        image_info.sampler = sampler;

        m_descriptors.update_descriptor_set(0, SHADOW_BINDING, image_info);
}
void RendererTrianglesSharedMemory::set_object_image(const vulkan::ImageWithMemory* storage_image) const
{
        ASSERT(storage_image && storage_image->format() == VK_FORMAT_R32_UINT);
        ASSERT(storage_image && (storage_image->usage() & VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = storage_image->image_view();

        m_descriptors.update_descriptor_set(0, OBJECTS_BINDING, image_info);
}

//

std::vector<VkDescriptorSetLayoutBinding> RendererTrianglesMaterialMemory::descriptor_set_layout_bindings()
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

RendererTrianglesMaterialMemory::RendererTrianglesMaterialMemory(
        const vulkan::Device& device,
        const std::unordered_set<uint32_t>& family_indices,
        VkSampler sampler,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<MaterialAndTexture>& materials)
        : m_descriptors(vulkan::Descriptors(
                  device,
                  materials.size(),
                  descriptor_set_layout,
                  descriptor_set_layout_bindings()))
{
        ASSERT(!materials.empty());
        ASSERT(std::all_of(materials.cbegin(), materials.cend(), [](const MaterialAndTexture& m) {
                return m.texture_Ka && m.texture_Kd && m.texture_Ks;
        }));

        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        for (size_t i = 0; i < materials.size(); ++i)
        {
                const MaterialAndTexture& material = materials[i];

                infos.clear();
                bindings.clear();
                {
                        m_uniform_buffers.emplace_back(
                                device, family_indices, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Material),
                                materials[i].material);

                        VkDescriptorBufferInfo buffer_info = {};
                        buffer_info.buffer = m_uniform_buffers.back();
                        buffer_info.offset = 0;
                        buffer_info.range = m_uniform_buffers.back().size();

                        infos.emplace_back(buffer_info);

                        bindings.push_back(MATERIAL_BINDING);
                }
                {
                        VkDescriptorImageInfo image_info = {};
                        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        image_info.imageView = material.texture_Ka->image_view();
                        image_info.sampler = sampler;

                        infos.emplace_back(image_info);

                        bindings.push_back(TEXTURE_KA_BINDING);
                }
                {
                        VkDescriptorImageInfo image_info = {};
                        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        image_info.imageView = material.texture_Kd->image_view();
                        image_info.sampler = sampler;

                        infos.emplace_back(image_info);

                        bindings.push_back(TEXTURE_KD_BINDING);
                }
                {
                        VkDescriptorImageInfo image_info = {};
                        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        image_info.imageView = material.texture_Ks->image_view();
                        image_info.sampler = sampler;

                        infos.emplace_back(image_info);

                        bindings.push_back(TEXTURE_KS_BINDING);
                }
                m_descriptors.update_descriptor_set(i, bindings, infos);
        }
}

unsigned RendererTrianglesMaterialMemory::set_number()
{
        return SET_NUMBER;
}

uint32_t RendererTrianglesMaterialMemory::descriptor_set_count() const
{
        return m_descriptors.descriptor_set_count();
}

const VkDescriptorSet& RendererTrianglesMaterialMemory::descriptor_set(uint32_t index) const
{
        return m_descriptors.descriptor_set(index);
}

//

std::vector<VkDescriptorSetLayoutBinding> RendererShadowMemory::descriptor_set_layout_bindings()
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

RendererShadowMemory::RendererShadowMemory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::unordered_set<uint32_t>& family_indices)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                m_uniform_buffers.emplace_back(
                        vulkan::BufferMemoryType::HostVisible, device, family_indices,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Matrices));

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.emplace_back(buffer_info);

                bindings.push_back(MATRICES_BINDING);
        }

        m_descriptors.update_descriptor_set(0, bindings, infos);
}

unsigned RendererShadowMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& RendererShadowMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void RendererShadowMemory::set_matrix(const mat4& mvp_matrix) const
{
        Matrices::M matrices;
        matrices.mvp_matrix = to_matrix<float>(mvp_matrix).transpose();
        vulkan::map_and_write_to_buffer(m_uniform_buffers[0], offsetof(Matrices, matrices), matrices);
}

void RendererShadowMemory::set_clip_plane(const vec4& equation, bool enabled) const
{
        Matrices::C clip_plane;
        clip_plane.equation = to_vector<float>(equation);
        clip_plane.enabled = enabled ? 1 : 0;
        vulkan::map_and_write_to_buffer(m_uniform_buffers[0], offsetof(Matrices, clip_plane), clip_plane);
}

//

std::vector<VkVertexInputBindingDescription> RendererTrianglesVertex::binding_descriptions()
{
        std::vector<VkVertexInputBindingDescription> descriptions;

        {
                VkVertexInputBindingDescription d = {};
                d.binding = 0;
                d.stride = sizeof(RendererTrianglesVertex);
                d.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                descriptions.push_back(d);
        }

        return descriptions;
}

std::vector<VkVertexInputAttributeDescription> RendererTrianglesVertex::triangles_attribute_descriptions()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;

        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 0;
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(RendererTrianglesVertex, position);

                descriptions.push_back(d);
        }
        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 1;
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(RendererTrianglesVertex, normal);

                descriptions.push_back(d);
        }
        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 2;
                d.format = VK_FORMAT_R32G32_SFLOAT;
                d.offset = offsetof(RendererTrianglesVertex, texture_coordinates);

                descriptions.push_back(d);
        }

        return descriptions;
}

std::vector<VkVertexInputAttributeDescription> RendererTrianglesVertex::shadow_attribute_descriptions()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;

        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 0;
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(RendererTrianglesVertex, position);

                descriptions.push_back(d);
        }

        return descriptions;
}

//

RendererTrianglesProgram::RendererTrianglesProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout_shared(vulkan::create_descriptor_set_layout(
                  device,
                  RendererTrianglesSharedMemory::descriptor_set_layout_bindings())),
          m_descriptor_set_layout_material(vulkan::create_descriptor_set_layout(
                  device,
                  RendererTrianglesMaterialMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(
                  device,
                  {RendererTrianglesSharedMemory::set_number(), RendererTrianglesMaterialMemory::set_number()},
                  {m_descriptor_set_layout_shared, m_descriptor_set_layout_material})),
          m_vertex_shader(m_device, renderer_triangles_vert(), "main"),
          m_geometry_shader(m_device, renderer_triangles_geom(), "main"),
          m_fragment_shader(m_device, renderer_triangles_frag(), "main")
{
}

VkDescriptorSetLayout RendererTrianglesProgram::descriptor_set_layout_shared() const
{
        return m_descriptor_set_layout_shared;
}

VkDescriptorSetLayout RendererTrianglesProgram::descriptor_set_layout_material() const
{
        return m_descriptor_set_layout_material;
}

VkPipelineLayout RendererTrianglesProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline RendererTrianglesProgram::create_pipeline(
        VkRenderPass render_pass,
        VkSampleCountFlagBits sample_count,
        bool sample_shading,
        unsigned x,
        unsigned y,
        unsigned width,
        unsigned height) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = sample_shading;
        info.pipeline_layout = m_pipeline_layout;
        info.viewport_x = x;
        info.viewport_y = y;
        info.viewport_width = width;
        info.viewport_height = height;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        info.depth_bias = false;
        info.color_blend = false;

        const std::vector<const vulkan::Shader*> shaders = {&m_vertex_shader, &m_geometry_shader, &m_fragment_shader};
        const std::vector<const vulkan::SpecializationConstant*> constants = {nullptr, nullptr, nullptr};
        const std::vector<VkVertexInputBindingDescription> binding_descriptions =
                RendererTrianglesVertex::binding_descriptions();
        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                RendererTrianglesVertex::triangles_attribute_descriptions();

        info.shaders = &shaders;
        info.constants = &constants;
        info.binding_descriptions = &binding_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}

//

RendererShadowProgram::RendererShadowProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, RendererShadowMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(
                  device,
                  {RendererShadowMemory::set_number()},
                  {m_descriptor_set_layout})),
          m_vertex_shader(m_device, renderer_shadow_vert(), "main"),
          m_fragment_shader(m_device, renderer_shadow_frag(), "main")
{
}

VkDescriptorSetLayout RendererShadowProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout RendererShadowProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline RendererShadowProgram::create_pipeline(
        VkRenderPass render_pass,
        VkSampleCountFlagBits sample_count,
        unsigned x,
        unsigned y,
        unsigned width,
        unsigned height) const
{
        ASSERT(sample_count = VK_SAMPLE_COUNT_1_BIT);
        ASSERT(x < width && y < height);

        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = false;
        info.pipeline_layout = m_pipeline_layout;
        info.viewport_x = x;
        info.viewport_y = y;
        info.viewport_width = width;
        info.viewport_height = height;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        info.depth_bias = true;
        info.color_blend = false;

        const std::vector<const vulkan::Shader*> shaders = {&m_vertex_shader, &m_fragment_shader};
        info.shaders = &shaders;

        const std::vector<const vulkan::SpecializationConstant*> constants = {nullptr, nullptr};
        info.constants = &constants;

        const std::vector<VkVertexInputBindingDescription> binding_descriptions =
                RendererTrianglesVertex::binding_descriptions();
        info.binding_descriptions = &binding_descriptions;

        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                RendererTrianglesVertex::shadow_attribute_descriptions();
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
