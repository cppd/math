/*
Copyright (C) 2017-2026 Topological Manifold

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
#include "shaders/vertex_triangles.h"

#include <src/model/mesh.h>
#include <src/vulkan/acceleration_structure.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace ns::gpu::renderer
{
inline constexpr VkIndexType VERTEX_INDEX_TYPE = VK_INDEX_TYPE_UINT32;
using VertexIndexType = std::uint32_t;

struct BufferMesh final
{
        std::vector<TrianglesVertex> vertices;
        std::vector<VertexIndexType> indices;
};

void load_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh,
        const std::vector<int>& sorted_face_indices,
        std::unique_ptr<vulkan::BufferWithMemory>* vertex_buffer,
        std::unique_ptr<vulkan::BufferWithMemory>* index_buffer,
        BufferMesh* buffer_mesh);

std::unique_ptr<vulkan::BottomLevelAccelerationStructure> load_acceleration_structure(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        const BufferMesh& buffer_mesh);

std::unique_ptr<vulkan::BufferWithMemory> load_point_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh);

std::unique_ptr<vulkan::BufferWithMemory> load_line_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh);

std::vector<vulkan::ImageWithMemory> load_textures(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh);

std::vector<MaterialBuffer> load_materials(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh);
}
