/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <src/model/mesh.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/objects.h>

#include <memory>
#include <vector>

namespace ns::gpu::renderer
{
inline constexpr VkIndexType VERTEX_INDEX_TYPE = VK_INDEX_TYPE_UINT32;

void load_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const mesh::Mesh<3>& mesh,
        const std::vector<int>& sorted_face_indices,
        std::unique_ptr<vulkan::BufferWithMemory>* vertex_buffer,
        std::unique_ptr<vulkan::BufferWithMemory>* index_buffer,
        unsigned* vertex_count,
        unsigned* index_count);

std::unique_ptr<vulkan::BufferWithMemory> load_point_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const mesh::Mesh<3>& mesh);

std::unique_ptr<vulkan::BufferWithMemory> load_line_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const mesh::Mesh<3>& mesh);

std::vector<vulkan::ImageWithMemory> load_textures(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const mesh::Mesh<3>& mesh);

std::vector<MaterialBuffer> load_materials(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const mesh::Mesh<3>& mesh);
}
