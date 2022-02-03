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

#include "shader.h"

#include "../../com/matrix.h"

namespace ns::gpu::renderer
{
ShaderBuffers::ShaderBuffers(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices)
{
        static_assert(MATRICES_INDEX == 0);
        static_assert(SHADOW_MATRICES_INDEX == 1);
        static_assert(DRAWING_INDEX == 2);

        static constexpr auto MEMORY_TYPE = vulkan::BufferMemoryType::HOST_VISIBLE;
        static constexpr auto USAGE = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

        uniform_buffers_.reserve(3);
        uniform_buffers_.emplace_back(MEMORY_TYPE, device, family_indices, USAGE, sizeof(Matrices));
        uniform_buffers_.emplace_back(MEMORY_TYPE, device, family_indices, USAGE, sizeof(Matrices));
        uniform_buffers_.emplace_back(MEMORY_TYPE, device, family_indices, USAGE, sizeof(Drawing));
}

const vulkan::Buffer& ShaderBuffers::matrices_buffer() const
{
        return uniform_buffers_[MATRICES_INDEX].buffer();
}

const vulkan::Buffer& ShaderBuffers::shadow_matrices_buffer() const
{
        return uniform_buffers_[SHADOW_MATRICES_INDEX].buffer();
}

const vulkan::Buffer& ShaderBuffers::drawing_buffer() const
{
        return uniform_buffers_[DRAWING_INDEX].buffer();
}

template <typename T>
void ShaderBuffers::copy_to_matrices_buffer(const VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(uniform_buffers_[MATRICES_INDEX], offset, data);
}

template <typename T>
void ShaderBuffers::copy_to_shadow_matrices_buffer(const VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(uniform_buffers_[SHADOW_MATRICES_INDEX], offset, data);
}

template <typename T>
void ShaderBuffers::copy_to_drawing_buffer(const VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(uniform_buffers_[DRAWING_INDEX], offset, data);
}

void ShaderBuffers::set_matrices(
        const Matrix4d& main_vp_matrix,
        const Matrix4d& shadow_vp_matrix,
        const Matrix4d& shadow_vp_texture_matrix) const
{
        {
                Matrices matrices;
                matrices.vp_matrix = to_std140<float>(main_vp_matrix);
                matrices.shadow_vp_texture_matrix = to_std140<float>(shadow_vp_texture_matrix);
                copy_to_matrices_buffer(0, matrices);
        }
        {
                Matrices matrices;
                matrices.vp_matrix = to_std140<float>(shadow_vp_matrix);
                matrices.shadow_vp_texture_matrix = to_std140<float>(shadow_vp_texture_matrix);
                copy_to_shadow_matrices_buffer(0, matrices);
        }
}

void ShaderBuffers::set_transparency_max_node_count(const std::uint32_t count) const
{
        decltype(Drawing().transparency_max_node_count) c = count;
        copy_to_drawing_buffer(offsetof(Drawing, transparency_max_node_count), c);
}

void ShaderBuffers::set_clip_plane(const Vector4d& equation, const bool enabled) const
{
        static_assert(
                offsetof(Drawing, clip_plane_equation) + sizeof(Drawing::clip_plane_equation)
                == offsetof(Drawing, clip_plane_enabled));

        constexpr std::size_t OFFSET = offsetof(Drawing, clip_plane_equation);
        constexpr std::size_t SIZE = sizeof(Drawing::clip_plane_equation) + sizeof(Drawing::clip_plane_enabled);

        vulkan::BufferMapper map(uniform_buffers_[DRAWING_INDEX], OFFSET, SIZE);

        decltype(Drawing().clip_plane_equation) clip_plane_equation = to_vector<float>(equation);
        decltype(Drawing().clip_plane_enabled) clip_plane_enabled = enabled ? 1 : 0;

        map.write(0, clip_plane_equation);
        map.write(sizeof(clip_plane_equation), clip_plane_enabled);
}

void ShaderBuffers::set_viewport(const Vector2d& center, const Vector2d& factor) const
{
        static_assert(
                offsetof(Drawing, viewport_center) + sizeof(Drawing::viewport_factor)
                == offsetof(Drawing, viewport_factor));

        constexpr std::size_t OFFSET = offsetof(Drawing, viewport_center);
        constexpr std::size_t SIZE = sizeof(Drawing::viewport_center) + sizeof(Drawing::viewport_factor);

        vulkan::BufferMapper map(uniform_buffers_[DRAWING_INDEX], OFFSET, SIZE);

        decltype(Drawing().viewport_center) viewport_center = to_vector<float>(center);
        decltype(Drawing().viewport_factor) viewport_factor = to_vector<float>(factor);

        map.write(0, viewport_center);
        map.write(sizeof(viewport_center), viewport_factor);
}

void ShaderBuffers::set_lighting_color(const Vector3f& color) const
{
        decltype(Drawing().lighting_color) v = color;
        copy_to_drawing_buffer(offsetof(Drawing, lighting_color), v);
}

void ShaderBuffers::set_background_color(const Vector3f& color) const
{
        decltype(Drawing().background_color) c = color;
        copy_to_drawing_buffer(offsetof(Drawing, background_color), c);
}

void ShaderBuffers::set_wireframe_color(const Vector3f& color) const
{
        decltype(Drawing().wireframe_color) c = color;
        copy_to_drawing_buffer(offsetof(Drawing, wireframe_color), c);
}

void ShaderBuffers::set_clip_plane_color(const Vector3f& color) const
{
        decltype(Drawing().clip_plane_color) c = color;
        copy_to_drawing_buffer(offsetof(Drawing, clip_plane_color), c);
}

void ShaderBuffers::set_normal_color_positive(const Vector3f& color) const
{
        decltype(Drawing().normal_color_positive) c = color;
        copy_to_drawing_buffer(offsetof(Drawing, normal_color_positive), c);
}

void ShaderBuffers::set_normal_color_negative(const Vector3f& color) const
{
        decltype(Drawing().normal_color_negative) c = color;
        copy_to_drawing_buffer(offsetof(Drawing, normal_color_negative), c);
}

void ShaderBuffers::set_normal_length(const float length) const
{
        decltype(Drawing().normal_length) l = length;
        copy_to_drawing_buffer(offsetof(Drawing, normal_length), l);
}

void ShaderBuffers::set_show_materials(const bool show) const
{
        decltype(Drawing().show_materials) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_materials), s);
}

void ShaderBuffers::set_show_wireframe(const bool show) const
{
        decltype(Drawing().show_wireframe) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_wireframe), s);
}

void ShaderBuffers::set_show_shadow(const bool show) const
{
        decltype(Drawing().show_shadow) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_shadow), s);
}

void ShaderBuffers::set_show_fog(const bool show) const
{
        decltype(Drawing().show_fog) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_fog), s);
}

void ShaderBuffers::set_show_smooth(const bool show) const
{
        decltype(Drawing().show_smooth) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_smooth), s);
}

void ShaderBuffers::set_direction_to_light(const Vector3f& direction) const
{
        decltype(Drawing().direction_to_light) d = direction;
        copy_to_drawing_buffer(offsetof(Drawing, direction_to_light), d);
}

void ShaderBuffers::set_direction_to_camera(const Vector3f& direction) const
{
        decltype(Drawing().direction_to_camera) d = direction;
        copy_to_drawing_buffer(offsetof(Drawing, direction_to_camera), d);
}
}
