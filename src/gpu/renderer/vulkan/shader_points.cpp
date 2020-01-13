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

#include "shader_points.h"

#include "shader_source.h"

#include "com/error.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"

namespace gpu_vulkan
{
std::vector<VkDescriptorSetLayoutBinding> RendererPointsMemory::descriptor_set_layout_bindings()
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

RendererPointsMemory::RendererPointsMemory(const vulkan::Device& device, VkDescriptorSetLayout descriptor_set_layout,
                                           const std::unordered_set<uint32_t>& family_indices)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<Variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                m_uniform_buffers.emplace_back(vulkan::BufferMemoryType::HostVisible, device, family_indices,
                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Matrices));
                m_matrices_buffer_index = m_uniform_buffers.size() - 1;

                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = m_uniform_buffers.back();
                buffer_info.offset = 0;
                buffer_info.range = m_uniform_buffers.back().size();

                infos.push_back(buffer_info);

                bindings.push_back(MATRICES_BINDING);
        }
        {
                m_uniform_buffers.emplace_back(vulkan::BufferMemoryType::HostVisible, device, family_indices,
                                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Drawing));
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

unsigned RendererPointsMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& RendererPointsMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

template <typename T>
void RendererPointsMemory::copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_matrices_buffer_index], offset, data);
}
template <typename T>
void RendererPointsMemory::copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_drawing_buffer_index], offset, data);
}

void RendererPointsMemory::set_matrix(const mat4& mvp_matrix) const
{
        Matrices::M matrices;
        matrices.mvp_matrix = to_matrix<float>(mvp_matrix).transpose();
        copy_to_matrices_buffer(offsetof(Matrices, matrices), matrices);
}

void RendererPointsMemory::set_clip_plane(const vec4& equation, bool enabled) const
{
        Matrices::C clip_plane;
        clip_plane.equation = to_vector<float>(equation);
        clip_plane.enabled = enabled ? 1 : 0;
        copy_to_matrices_buffer(offsetof(Matrices, clip_plane), clip_plane);
}

void RendererPointsMemory::set_default_color(const Color& color) const
{
        decltype(Drawing().default_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, default_color), c);
}
void RendererPointsMemory::set_background_color(const Color& color) const
{
        decltype(Drawing().background_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, background_color), c);
}
void RendererPointsMemory::set_light_a(const Color& color) const
{
        decltype(Drawing().light_a) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_a), c);
}
void RendererPointsMemory::set_show_fog(bool show) const
{
        decltype(Drawing().show_fog) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_fog), s);
}
void RendererPointsMemory::set_object_image(const vulkan::ImageWithMemory* storage_image) const
{
        ASSERT(storage_image && storage_image->format() == VK_FORMAT_R32_UINT);
        ASSERT(storage_image && (storage_image->usage() & VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = storage_image->image_view();

        m_descriptors.update_descriptor_set(0, OBJECTS_BINDING, image_info);
}

//

std::vector<VkVertexInputBindingDescription> RendererPointsVertex::binding_descriptions()
{
        std::vector<VkVertexInputBindingDescription> descriptions;

        {
                VkVertexInputBindingDescription d = {};
                d.binding = 0;
                d.stride = sizeof(RendererPointsVertex);
                d.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

                descriptions.push_back(d);
        }

        return descriptions;
}

std::vector<VkVertexInputAttributeDescription> RendererPointsVertex::attribute_descriptions()
{
        std::vector<VkVertexInputAttributeDescription> descriptions;

        {
                VkVertexInputAttributeDescription d = {};
                d.binding = 0;
                d.location = 0;
                d.format = VK_FORMAT_R32G32B32_SFLOAT;
                d.offset = offsetof(RendererPointsVertex, position);

                descriptions.push_back(d);
        }

        return descriptions;
}

//

RendererPointsProgram::RendererPointsProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout(
                  vulkan::create_descriptor_set_layout(device, RendererPointsMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(
                  vulkan::create_pipeline_layout(device, {RendererPointsMemory::set_number()}, {m_descriptor_set_layout})),
          m_vertex_shader_0d(m_device, renderer_points_0d_vert(), "main"),
          m_vertex_shader_1d(m_device, renderer_points_1d_vert(), "main"),
          m_fragment_shader(m_device, renderer_points_frag(), "main")
{
}

VkDescriptorSetLayout RendererPointsProgram::descriptor_set_layout() const
{
        return m_descriptor_set_layout;
}

VkPipelineLayout RendererPointsProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline RendererPointsProgram::create_pipeline(VkRenderPass render_pass, VkSampleCountFlagBits sample_count,
                                                        VkPrimitiveTopology primitive_topology, unsigned x, unsigned y,
                                                        unsigned width, unsigned height) const
{
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
        info.primitive_topology = primitive_topology;
        info.depth_bias = false;
        info.color_blend = false;

        std::vector<const vulkan::Shader*> shaders;
        if (primitive_topology == VK_PRIMITIVE_TOPOLOGY_POINT_LIST)
        {
                shaders = {&m_vertex_shader_0d, &m_fragment_shader};
        }
        else if (primitive_topology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST)
        {
                shaders = {&m_vertex_shader_1d, &m_fragment_shader};
        }
        else
        {
                error_fatal("Unsupported primitive topology for renderer points program");
        }
        const std::vector<const vulkan::SpecializationConstant*> constants = {nullptr, nullptr};
        const std::vector<VkVertexInputBindingDescription> binding_descriptions = RendererPointsVertex::binding_descriptions();
        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                RendererPointsVertex::attribute_descriptions();

        info.shaders = &shaders;
        info.constants = &constants;
        info.binding_descriptions = &binding_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
