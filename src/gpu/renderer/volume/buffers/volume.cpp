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

#include "volume.h"

#include <src/com/container.h>
#include <src/com/error.h>
#include <src/com/merge.h>
#include <src/geometry/spatial/hyperplane.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/layout.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
namespace
{
template <typename T>
numerical::Vector<4, T> clip_plane_to_buffer_vector(const geometry::spatial::Hyperplane<3, double>& clip_plane)
{
        return {clip_plane.n[0], clip_plane.n[1], clip_plane.n[2], clip_plane.d};
}
}

VolumeBuffer::VolumeBuffer(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& graphics_family_indices,
        const std::vector<std::uint32_t>& transfer_family_indices)
        : uniform_buffer_coordinates_(
                  vulkan::BufferMemoryType::HOST_VISIBLE,
                  device,
                  graphics_family_indices,
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  sizeof(VolumeCoordinates)),
          uniform_buffer_volume_(
                  vulkan::BufferMemoryType::DEVICE_LOCAL,
                  device,
                  merge<std::vector<std::uint32_t>>(graphics_family_indices, transfer_family_indices),
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  sizeof(Volume))
{
}

const vulkan::Buffer& VolumeBuffer::buffer_coordinates() const
{
        return uniform_buffer_coordinates_.buffer();
}

const vulkan::Buffer& VolumeBuffer::buffer_volume() const
{
        return uniform_buffer_volume_.buffer();
}

void VolumeBuffer::set_coordinates(
        const numerical::Matrix4d& device_to_texture_matrix,
        const numerical::Matrix4d& texture_to_world_matrix,
        const numerical::Vector4d& third_row_of_texture_to_device,
        const geometry::spatial::Hyperplane<3, double>& clip_plane,
        const numerical::Vector3d& gradient_h,
        const numerical::Matrix3d& gradient_to_world_matrix,
        const numerical::Matrix4d& world_to_texture_matrix) const
{
        VolumeCoordinates coordinates;
        coordinates.device_to_texture_matrix = vulkan::to_std140<float>(device_to_texture_matrix);
        coordinates.texture_to_world_matrix = vulkan::to_std140<float>(texture_to_world_matrix);
        coordinates.third_row_of_texture_to_device = to_vector<float>(third_row_of_texture_to_device);
        coordinates.clip_plane = clip_plane_to_buffer_vector<float>(clip_plane);
        coordinates.gradient_h = to_vector<float>(gradient_h);
        coordinates.gradient_to_world_matrix = vulkan::to_std140<float>(gradient_to_world_matrix);
        coordinates.world_to_texture_matrix = vulkan::to_std140<float>(world_to_texture_matrix);
        vulkan::map_and_write_to_buffer(uniform_buffer_coordinates_, 0, coordinates);
}

void VolumeBuffer::set_texture_to_shadow_matrix(const numerical::Matrix4d& texture_to_shadow_matrix) const
{
        const decltype(VolumeCoordinates().texture_to_shadow_matrix) m =
                vulkan::to_std140<float>(texture_to_shadow_matrix);
        vulkan::map_and_write_to_buffer(
                uniform_buffer_coordinates_, offsetof(VolumeCoordinates, texture_to_shadow_matrix), m);
}

void VolumeBuffer::set_clip_plane(const geometry::spatial::Hyperplane<3, double>& clip_plane) const
{
        const decltype(VolumeCoordinates().clip_plane) cp = clip_plane_to_buffer_vector<float>(clip_plane);
        vulkan::map_and_write_to_buffer(uniform_buffer_coordinates_, offsetof(VolumeCoordinates, clip_plane), cp);
}

void VolumeBuffer::set_parameters(
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const float window_offset,
        const float window_scale,
        const float volume_alpha_coefficient,
        const float isosurface_alpha,
        const bool isosurface,
        const float isovalue,
        const numerical::Vector3f& color) const
{
        ASSERT(window_offset >= 0);
        ASSERT(window_scale > 0);
        ASSERT(volume_alpha_coefficient >= 0);
        ASSERT(isosurface_alpha >= 0 && isosurface_alpha <= 1);
        ASSERT(isovalue >= 0 && isovalue <= 1);

        static_assert(offsetof(Volume, color) - offsetof(Volume, window_offset) == 8 * sizeof(float));

        constexpr std::size_t OFFSET = offsetof(Volume, window_offset);
        constexpr std::size_t SIZE = offsetof(Volume, color) + sizeof(Volume::color) - OFFSET;

        Volume volume;

        volume.window_offset = window_offset;
        volume.window_scale = window_scale;
        volume.volume_alpha_coefficient = volume_alpha_coefficient;
        volume.isosurface_alpha = isosurface_alpha;
        volume.isosurface = isosurface ? 1 : 0;
        volume.isovalue = isovalue;
        volume.color = color;

        uniform_buffer_volume_.write(
                command_pool, queue, OFFSET, SIZE, reinterpret_cast<const char*>(&volume) + OFFSET);
}

void VolumeBuffer::set_color_volume(
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const bool color_volume) const
{
        decltype(Volume().color_volume) v = color_volume ? 1 : 0;
        uniform_buffer_volume_.write(
                command_pool, queue, offsetof(Volume, color_volume), data_size(v), data_pointer(v));
}

void VolumeBuffer::set_lighting(
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const float ambient,
        const float metalness,
        const float roughness) const
{
        static_assert(offsetof(Volume, roughness) - offsetof(Volume, ambient) == 2 * sizeof(float));

        constexpr std::size_t OFFSET = offsetof(Volume, ambient);
        constexpr std::size_t SIZE = offsetof(Volume, roughness) + sizeof(Volume::roughness) - OFFSET;

        Volume volume;

        volume.ambient = ambient;
        volume.metalness = metalness;
        volume.roughness = roughness;

        uniform_buffer_volume_.write(
                command_pool, queue, OFFSET, SIZE, reinterpret_cast<const char*>(&volume) + OFFSET);
}
}
