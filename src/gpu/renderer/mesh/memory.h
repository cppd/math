/*
Copyright (C) 2017-2024 Topological Manifold

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

#pragma once

#include "buffers/material.h"
#include "shaders/descriptors.h"

#include <src/model/mesh.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <unordered_map>
#include <vector>

namespace ns::gpu::renderer
{
std::unordered_map<VkDescriptorSetLayout, MeshMemory> create_mesh_memory(
        VkDevice device,
        const std::vector<vulkan::DescriptorSetLayoutAndBindings>& mesh_layouts,
        const vulkan::Buffer& mesh_buffer);

std::unordered_map<VkDescriptorSetLayout, MaterialMemory> create_material_memory(
        VkDevice device,
        VkSampler texture_sampler,
        const std::vector<vulkan::DescriptorSetLayoutAndBindings>& material_layouts,
        const model::mesh::Mesh<3>& mesh,
        const std::vector<vulkan::ImageWithMemory>& textures,
        const std::vector<MaterialBuffer>& material_buffers);
}
