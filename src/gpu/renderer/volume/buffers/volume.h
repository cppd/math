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

#include <src/gpu/com/matrix.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>

#include <vector>

namespace ns::gpu::renderer
{
class VolumeBuffer final
{
        struct Coordinates final
        {
                alignas(sizeof(Vector4f)) Matrix4f inverse_mvp_matrix;
                alignas(sizeof(Vector4f)) Matrix4f texture_to_world_matrix;
                alignas(sizeof(Vector4f)) Matrix4f device_to_world_matrix;
                alignas(sizeof(Vector4f)) Vector4f third_row_of_mvp;
                alignas(sizeof(Vector4f)) Vector4f clip_plane_equation;
                alignas(sizeof(Vector4f)) Vector3f gradient_h;
                alignas(sizeof(Vector4f)) std140::Matrix3f gradient_to_world_matrix;
                alignas(sizeof(Vector4f)) std140::Matrix3f world_to_texture_matrix;
        };

        struct Volume final
        {
                float window_offset;
                float window_scale;
                float volume_alpha_coefficient;
                float isosurface_alpha;
                std::uint32_t isosurface;
                float isovalue;
                alignas(sizeof(Vector4f)) Vector3f color;
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

        const vulkan::Buffer& buffer_coordinates() const;
        const vulkan::Buffer& buffer_volume() const;

        void set_coordinates(
                const Matrix4d& inverse_mvp_matrix,
                const Matrix4d& texture_to_world_matrix,
                const Matrix4d& device_to_world_matrix,
                const Vector4d& third_row_of_mvp,
                const Vector4d& clip_plane_equation,
                const Vector3d& gradient_h,
                const Matrix3d& gradient_to_world_matrix,
                const Matrix3d& world_to_texture_matrix) const;

        void set_clip_plane(const Vector4d& clip_plane_equation) const;

        void set_parameters(
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                float window_offset,
                float window_scale,
                float volume_alpha_coefficient,
                float isosurface_alpha,
                bool isosurface,
                float isovalue,
                const Vector3f& color) const;

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
