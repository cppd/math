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

#include "load.h"

#include "buffers/material.h"
#include "shaders/vertex_points.h"
#include "shaders/vertex_triangles.h"

#include <src/com/chrono.h>
#include <src/com/container.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/image/image.h>
#include <src/model/mesh.h>
#include <src/numerical/vector.h>
#include <src/vulkan/acceleration_structure.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ns::gpu::renderer
{
namespace
{
// clang-format off
constexpr std::array COLOR_IMAGE_FORMATS
{
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R32G32B32A32_SFLOAT
};
// clang-format on

std::unique_ptr<vulkan::BufferWithMemory> make_vertex_buffer(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const std::vector<PointsVertex>& vertices)
{
        auto buffer = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::DEVICE_LOCAL, device, family_indices,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, data_size(vertices));

        buffer->write(command_pool, queue, data_size(vertices), data_pointer(vertices));

        return buffer;
}
}

std::unique_ptr<vulkan::BufferWithMemory> load_point_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh)
{
        if (mesh.points.empty())
        {
                return {};
        }

        std::vector<PointsVertex> vertices;
        vertices.reserve(mesh.points.size());

        for (const model::mesh::Mesh<3>::Point& p : mesh.points)
        {
                vertices.emplace_back(mesh.vertices[p.vertex]);
        }

        return make_vertex_buffer(device, command_pool, queue, family_indices, vertices);
}

std::unique_ptr<vulkan::BufferWithMemory> load_line_vertices(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh)
{
        if (mesh.lines.empty())
        {
                return {};
        }

        std::vector<PointsVertex> vertices;
        vertices.reserve(2 * mesh.lines.size());

        for (const model::mesh::Mesh<3>::Line& line : mesh.lines)
        {
                for (const int index : line.vertices)
                {
                        vertices.emplace_back(mesh.vertices[index]);
                }
        }

        return make_vertex_buffer(device, command_pool, queue, family_indices, vertices);
}

std::vector<vulkan::ImageWithMemory> load_textures(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh)
{
        const std::vector<VkFormat> formats(std::cbegin(COLOR_IMAGE_FORMATS), std::cend(COLOR_IMAGE_FORMATS));

        std::vector<vulkan::ImageWithMemory> textures;

        for (const image::Image<2>& image : mesh.images)
        {
                textures.emplace_back(
                        device, family_indices, formats, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D,
                        vulkan::make_extent(image.size[0], image.size[1]),
                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                        command_pool, queue);
                textures.back().write(
                        command_pool, queue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        image.color_format, image.pixels);
        }

        // texture for materials without texture
        textures.emplace_back(
                device, family_indices, formats, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_2D, vulkan::make_extent(1, 1),
                VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, command_pool, queue);

        return textures;
}

std::vector<MaterialBuffer> load_materials(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const model::mesh::Mesh<3>& mesh)
{
        std::vector<MaterialBuffer> buffers;
        buffers.reserve(mesh.materials.size() + 1);

        for (const model::mesh::Mesh<3>::Material& mesh_material : mesh.materials)
        {
                const numerical::Vector3f color = mesh_material.color.rgb32().clamp(0, 1);
                const bool use_texture = (mesh_material.image >= 0);
                const bool use_material = true;
                buffers.emplace_back(device, command_pool, queue, family_indices, color, use_texture, use_material);
        }

        // material for vertices without material
        constexpr numerical::Vector3f COLOR(0);
        constexpr bool USE_TEXTURE = false;
        constexpr bool USE_MATERIAL = false;
        buffers.emplace_back(device, command_pool, queue, family_indices, COLOR, USE_TEXTURE, USE_MATERIAL);

        return buffers;
}

std::unique_ptr<vulkan::BottomLevelAccelerationStructure> load_acceleration_structure(
        const vulkan::Device& device,
        const vulkan::CommandPool& compute_command_pool,
        const vulkan::Queue& compute_queue,
        const std::vector<std::uint32_t>& family_indices,
        const std::vector<TrianglesVertex>& vertices,
        const std::vector<std::uint32_t>& indices)
{
        if (indices.empty())
        {
                return {};
        }

        const Clock::time_point start_time = Clock::now();

        std::vector<numerical::Vector3f> positions;
        positions.reserve(vertices.size());
        for (const TrianglesVertex& v : vertices)
        {
                positions.push_back(v.position);
        }

        vulkan::BottomLevelAccelerationStructure acceleration_structure =
                vulkan::create_bottom_level_acceleration_structure(
                        device, compute_command_pool, compute_queue, family_indices, positions, indices, std::nullopt);

        const double duration = duration_from(start_time);

        LOG("Mesh acceleration structure info: " + to_string_fixed(1000.0 * duration, 5) + " ms");

        return std::make_unique<vulkan::BottomLevelAccelerationStructure>(std::move(acceleration_structure));
}
}
