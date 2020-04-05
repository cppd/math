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
#include "shader_vertex.h"

#include <src/com/error.h>
#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

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
        const RendererBuffers& buffers)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
{
        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = buffers.matrices_buffer();
                buffer_info.offset = 0;
                buffer_info.range = buffers.matrices_size();

                infos.emplace_back(buffer_info);

                bindings.push_back(MATRICES_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = buffers.lighting_buffer();
                buffer_info.offset = 0;
                buffer_info.range = buffers.lighting_size();

                infos.emplace_back(buffer_info);

                bindings.push_back(LIGHTING_BINDING);
        }
        {
                VkDescriptorBufferInfo buffer_info = {};
                buffer_info.buffer = buffers.drawing_buffer();
                buffer_info.offset = 0;
                buffer_info.range = buffers.drawing_size();

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

        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
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
        const Region<2, int>& viewport) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = sample_shading;
        info.pipeline_layout = m_pipeline_layout;
        info.viewport = viewport;
        info.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        info.depth_bias = false;
        info.color_blend = false;

        const std::vector<const vulkan::Shader*> shaders = {&m_vertex_shader, &m_geometry_shader, &m_fragment_shader};
        const std::vector<const vulkan::SpecializationConstant*> constants = {nullptr, nullptr, nullptr};
        const std::vector<VkVertexInputBindingDescription> binding_descriptions =
                RendererTrianglesVertex::binding_descriptions();
        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                RendererTrianglesVertex::attribute_descriptions_triangles();

        info.shaders = &shaders;
        info.constants = &constants;
        info.binding_descriptions = &binding_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
