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

#include <src/geometry/spatial/hyperplane.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/layout.h>
#include <src/vulkan/objects.h>

#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
class VolumeBuffer final
{
        struct VolumeCoordinates final
        {
                vulkan::std140::Matrix4f device_to_texture_matrix;
                vulkan::std140::Matrix4f texture_to_world_matrix;
                vulkan::std140::Matrix4f texture_to_shadow_matrix;
                vulkan::std140::Vector4f third_row_of_texture_to_device;
                vulkan::std140::Vector4f clip_plane;
                vulkan::std140::Vector3f gradient_h;
                vulkan::std140::Matrix3f gradient_to_world_matrix;
                vulkan::std140::Matrix4f world_to_texture_matrix;
        };

        struct Volume final
        {
                float window_offset;
                float window_scale;
                float volume_alpha_coefficient;
                float isosurface_alpha;
                std::uint32_t isosurface;
                float isovalue;
                vulkan::std140::Vector3f color;
                std::uint32_t color_volume;
                float ambient;
                float metalness;
                float roughness;
        };

        vulkan::BufferWithMemory uniform_buffer_coordinates_;
        vulkan::BufferWithMemory uniform_buffer_volume_;

public:
        VolumeBuffer(
                const vulkan::Device& device,
                const std::vector<std::uint32_t>& graphics_family_indices,
                const std::vector<std::uint32_t>& transfer_family_indices);

        [[nodiscard]] const vulkan::Buffer& buffer_coordinates() const;
        [[nodiscard]] const vulkan::Buffer& buffer_volume() const;

        void set_coordinates(
                const numerical::Matrix4d& device_to_texture_matrix,
                const numerical::Matrix4d& texture_to_world_matrix,
                const numerical::Vector4d& third_row_of_texture_to_device,
                const geometry::spatial::Hyperplane<3, double>& clip_plane,
                const numerical::Vector3d& gradient_h,
                const numerical::Matrix3d& gradient_to_world_matrix,
                const numerical::Matrix4d& world_to_texture_matrix) const;

        void set_texture_to_shadow_matrix(const numerical::Matrix4d& texture_to_shadow_matrix) const;

        void set_clip_plane(const geometry::spatial::Hyperplane<3, double>& clip_plane) const;

        void set_parameters(
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                float window_offset,
                float window_scale,
                float volume_alpha_coefficient,
                float isosurface_alpha,
                bool isosurface,
                float isovalue,
                const numerical::Vector3f& color) const;

        void set_color_volume(const vulkan::CommandPool& command_pool, const vulkan::Queue& queue, bool color_volume)
                const;

        void set_lighting(
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                float ambient,
                float metalness,
                float roughness) const;
};
}
