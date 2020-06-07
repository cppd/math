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

#include "points.h"

#include "vertex_points.h"

#include "code/code.h"

#include <src/com/error.h>
#include <src/vulkan/create.h>
#include <src/vulkan/pipeline.h>

namespace gpu::renderer
{
std::vector<VkDescriptorSetLayoutBinding> PointsSharedMemory::descriptor_set_layout_bindings()
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
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

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

PointsSharedMemory::PointsSharedMemory(
        const vulkan::Device& device,
        VkDescriptorSetLayout descriptor_set_layout,
        const vulkan::Buffer& matrices,
        const vulkan::Buffer& drawing)
        : m_descriptors(device, 1, descriptor_set_layout, descriptor_set_layout_bindings())
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

unsigned PointsSharedMemory::set_number()
{
        return SET_NUMBER;
}

const VkDescriptorSet& PointsSharedMemory::descriptor_set() const
{
        return m_descriptors.descriptor_set(0);
}

void PointsSharedMemory::set_object_image(const vulkan::ImageWithMemory* storage_image) const
{
        ASSERT(storage_image && storage_image->format() == VK_FORMAT_R32_UINT);
        ASSERT(storage_image && (storage_image->usage() & VK_IMAGE_USAGE_STORAGE_BIT));

        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        image_info.imageView = storage_image->image_view();

        m_descriptors.update_descriptor_set(0, OBJECTS_BINDING, image_info);
}

//

std::vector<VkDescriptorSetLayoutBinding> PointsMeshMemory::descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        {
                VkDescriptorSetLayoutBinding b = {};
                b.binding = BUFFER_BINDING;
                b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                b.descriptorCount = 1;
                b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

                bindings.push_back(b);
        }

        return bindings;
}

vulkan::Descriptors PointsMeshMemory::create(
        VkDevice device,
        VkDescriptorSetLayout descriptor_set_layout,
        const std::vector<CoordinatesInfo>& coordinates)
{
        ASSERT(!coordinates.empty());
        ASSERT(std::all_of(coordinates.cbegin(), coordinates.cend(), [](const CoordinatesInfo& m) {
                return m.buffer != VK_NULL_HANDLE;
        }));

        vulkan::Descriptors descriptors(vulkan::Descriptors(
                device, coordinates.size(), descriptor_set_layout, descriptor_set_layout_bindings()));

        std::vector<std::variant<VkDescriptorBufferInfo, VkDescriptorImageInfo>> infos;
        std::vector<uint32_t> bindings;

        for (size_t i = 0; i < coordinates.size(); ++i)
        {
                const CoordinatesInfo& coordinates_info = coordinates[i];

                infos.clear();
                bindings.clear();
                {
                        VkDescriptorBufferInfo buffer_info = {};
                        buffer_info.buffer = coordinates_info.buffer;
                        buffer_info.offset = 0;
                        buffer_info.range = coordinates_info.buffer_size;

                        infos.emplace_back(buffer_info);

                        bindings.push_back(BUFFER_BINDING);
                }
                descriptors.update_descriptor_set(i, bindings, infos);
        }

        return descriptors;
}

unsigned PointsMeshMemory::set_number()
{
        return SET_NUMBER;
}

//

PointsProgram::PointsProgram(const vulkan::Device& device)
        : m_device(device),
          m_descriptor_set_layout_shared(
                  vulkan::create_descriptor_set_layout(device, PointsSharedMemory::descriptor_set_layout_bindings())),
          m_descriptor_set_layout_mesh(
                  vulkan::create_descriptor_set_layout(device, PointsMeshMemory::descriptor_set_layout_bindings())),
          m_pipeline_layout(vulkan::create_pipeline_layout(
                  device,
                  {PointsSharedMemory::set_number(), PointsMeshMemory::set_number()},
                  {m_descriptor_set_layout_shared, m_descriptor_set_layout_mesh})),
          m_vertex_shader_0d(m_device, code_points_0d_vert(), "main"),
          m_vertex_shader_1d(m_device, code_points_1d_vert(), "main"),
          m_fragment_shader(m_device, code_points_frag(), "main")
{
}

VkDescriptorSetLayout PointsProgram::descriptor_set_layout_shared() const
{
        return m_descriptor_set_layout_shared;
}

VkDescriptorSetLayout PointsProgram::descriptor_set_layout_mesh() const
{
        return m_descriptor_set_layout_mesh;
}

VkPipelineLayout PointsProgram::pipeline_layout() const
{
        return m_pipeline_layout;
}

vulkan::Pipeline PointsProgram::create_pipeline(
        VkRenderPass render_pass,
        VkSampleCountFlagBits sample_count,
        VkPrimitiveTopology primitive_topology,
        const Region<2, int>& viewport) const
{
        vulkan::GraphicsPipelineCreateInfo info;

        info.device = &m_device;
        info.render_pass = render_pass;
        info.sub_pass = 0;
        info.sample_count = sample_count;
        info.sample_shading = false;
        info.pipeline_layout = m_pipeline_layout;
        info.viewport = viewport;
        info.primitive_topology = primitive_topology;

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
        const std::vector<VkVertexInputBindingDescription> binding_descriptions = PointsVertex::binding_descriptions();
        const std::vector<VkVertexInputAttributeDescription> attribute_descriptions =
                PointsVertex::attribute_descriptions();

        info.shaders = &shaders;
        info.constants = &constants;
        info.binding_descriptions = &binding_descriptions;
        info.attribute_descriptions = &attribute_descriptions;

        return vulkan::create_graphics_pipeline(info);
}
}
