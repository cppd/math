/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "buffers/material.h"
#include "shaders/descriptors.h"

#include <src/com/error.h>
#include <src/model/mesh.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ns::gpu::renderer
{
namespace
{
std::vector<MaterialMemory::MaterialInfo> materials_info(
        const model::mesh::Mesh<3>& mesh,
        const std::vector<vulkan::ImageWithMemory>& textures,
        const std::vector<MaterialBuffer>& material_buffers)
{
        // one more texture and material for specifying but not using
        ASSERT(textures.size() == mesh.images.size() + 1);
        ASSERT(material_buffers.size() == mesh.materials.size() + 1);

        const VkImageView no_texture = textures.back().image_view().handle();

        std::vector<MaterialMemory::MaterialInfo> res;
        res.reserve(mesh.materials.size() + 1);

        for (std::size_t i = 0; i < mesh.materials.size(); ++i)
        {
                const model::mesh::Mesh<3>::Material& mesh_material = mesh.materials[i];

                ASSERT(mesh_material.image < static_cast<int>(textures.size()) - 1);

                MaterialMemory::MaterialInfo& m = res.emplace_back();
                m.buffer = material_buffers[i].buffer().handle();
                m.buffer_size = material_buffers[i].buffer().size();
                m.texture =
                        (mesh_material.image >= 0) ? textures[mesh_material.image].image_view().handle() : no_texture;
        }

        MaterialMemory::MaterialInfo& m = res.emplace_back();
        m.buffer = material_buffers.back().buffer().handle();
        m.buffer_size = material_buffers.back().buffer().size();
        m.texture = no_texture;

        return res;
}
}

std::unordered_map<VkDescriptorSetLayout, MeshMemory> create_mesh_memory(
        const VkDevice device,
        const std::vector<vulkan::DescriptorSetLayoutAndBindings>& mesh_layouts,
        const vulkan::Buffer& mesh_buffer)
{
        std::unordered_map<VkDescriptorSetLayout, MeshMemory> res;

        for (const vulkan::DescriptorSetLayoutAndBindings& layout : mesh_layouts)
        {
                MeshMemory memory(
                        device, layout.descriptor_set_layout, layout.descriptor_set_layout_bindings, mesh_buffer);
                res.emplace(layout.descriptor_set_layout, std::move(memory));
        }

        return res;
}

std::unordered_map<VkDescriptorSetLayout, MaterialMemory> create_material_memory(
        const VkDevice device,
        const VkSampler texture_sampler,
        const std::vector<vulkan::DescriptorSetLayoutAndBindings>& material_layouts,
        const model::mesh::Mesh<3>& mesh,
        const std::vector<vulkan::ImageWithMemory>& textures,
        const std::vector<MaterialBuffer>& material_buffers)
{
        const std::vector<MaterialMemory::MaterialInfo> material_info =
                materials_info(mesh, textures, material_buffers);

        if (material_info.empty())
        {
                return {};
        }

        std::unordered_map<VkDescriptorSetLayout, MaterialMemory> res;

        for (const vulkan::DescriptorSetLayoutAndBindings& layout : material_layouts)
        {
                MaterialMemory memory(
                        device, texture_sampler, layout.descriptor_set_layout, layout.descriptor_set_layout_bindings,
                        material_info);
                res.emplace(layout.descriptor_set_layout, std::move(memory));
        }

        return res;
}
}
