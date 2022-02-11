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

#include "volume.h"

#include <src/com/merge.h>
#include <src/gpu/com/matrix.h>

namespace ns::gpu::renderer
{
VolumeBuffer::VolumeBuffer(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& graphics_family_indices,
        const std::vector<std::uint32_t>& transfer_family_indices)
        : uniform_buffer_coordinates_(
                vulkan::BufferMemoryType::HOST_VISIBLE,
                device,
                graphics_family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Coordinates)),
          uniform_buffer_volume_(
                  vulkan::BufferMemoryType::DEVICE_LOCAL,
                  device,
                  merge<std::vector<std::uint32_t>>(graphics_family_indices, transfer_family_indices),
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  sizeof(Volume))
{
}

VkBuffer VolumeBuffer::buffer_coordinates() const
{
        return uniform_buffer_coordinates_.buffer();
}

VkDeviceSize VolumeBuffer::buffer_coordinates_size() const
{
        return uniform_buffer_coordinates_.buffer().size();
}

VkBuffer VolumeBuffer::buffer_volume() const
{
        return uniform_buffer_volume_.buffer();
}

VkDeviceSize VolumeBuffer::buffer_volume_size() const
{
        return uniform_buffer_volume_.buffer().size();
}

void VolumeBuffer::set_coordinates(
        const Matrix4d& inverse_mvp_matrix,
        const Matrix4d& texture_to_world_matrix,
        const Vector4d& third_row_of_mvp,
        const Vector4d& clip_plane_equation,
        const Vector3d& gradient_h,
        const Matrix3d& gradient_to_world_matrix) const
{
        Coordinates coordinates;
        coordinates.inverse_mvp_matrix = to_std140<float>(inverse_mvp_matrix);
        coordinates.texture_to_world_matrix = to_std140<float>(texture_to_world_matrix);
        coordinates.third_row_of_mvp = to_vector<float>(third_row_of_mvp);
        coordinates.clip_plane_equation = to_vector<float>(clip_plane_equation);
        coordinates.gradient_h = to_vector<float>(gradient_h);
        coordinates.gradient_to_world_matrix = to_std140<float>(gradient_to_world_matrix);
        vulkan::map_and_write_to_buffer(uniform_buffer_coordinates_, 0, coordinates);
}

void VolumeBuffer::set_clip_plane(const Vector4d& clip_plane_equation) const
{
        decltype(Coordinates().clip_plane_equation) clip_plane = to_vector<float>(clip_plane_equation);
        vulkan::map_and_write_to_buffer(
                uniform_buffer_coordinates_, offsetof(Coordinates, clip_plane_equation), clip_plane);
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
        const Vector3f& color) const
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
