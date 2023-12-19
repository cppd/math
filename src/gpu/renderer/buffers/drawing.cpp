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

#include "drawing.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/layout.h>
#include <src/vulkan/objects.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ns::gpu::renderer
{
DrawingBuffer::DrawingBuffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices)
        : buffer_(
                vulkan::BufferMemoryType::HOST_VISIBLE,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Drawing))
{
}

const vulkan::Buffer& DrawingBuffer::buffer() const
{
        return buffer_.buffer();
}

template <typename T>
void DrawingBuffer::copy_to_buffer(const VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(buffer_, offset, data);
}

void DrawingBuffer::set_matrix(const Matrix4d& vp_matrix) const
{
        const decltype(Drawing().vp_matrix) m = vulkan::to_std140<float>(vp_matrix);
        copy_to_buffer(offsetof(Drawing, vp_matrix), m);
}

void DrawingBuffer::set_transparency_max_node_count(const std::uint32_t count) const
{
        const decltype(Drawing().transparency_max_node_count) c = count;
        copy_to_buffer(offsetof(Drawing, transparency_max_node_count), c);
}

void DrawingBuffer::set_clip_plane(const Vector4d& equation, const bool enabled) const
{
        static_assert(
                offsetof(Drawing, clip_plane_enabled) + sizeof(Drawing::clip_plane_enabled)
                == offsetof(Drawing, clip_plane_equation));

        constexpr std::size_t OFFSET = offsetof(Drawing, clip_plane_enabled);
        constexpr std::size_t SIZE = sizeof(Drawing::clip_plane_enabled) + sizeof(Drawing::clip_plane_equation);

        const vulkan::BufferMapper map(buffer_, OFFSET, SIZE);

        const decltype(Drawing().clip_plane_enabled) clip_plane_enabled = enabled ? 1 : 0;
        const decltype(Drawing().clip_plane_equation) clip_plane_equation = to_vector<float>(equation);

        map.write(0, clip_plane_enabled);
        map.write(sizeof(clip_plane_enabled), clip_plane_equation);
}

void DrawingBuffer::set_viewport(const Vector2d& center, const Vector2d& factor) const
{
        static_assert(
                offsetof(Drawing, viewport_center) + sizeof(Drawing::viewport_factor)
                == offsetof(Drawing, viewport_factor));

        constexpr std::size_t OFFSET = offsetof(Drawing, viewport_center);
        constexpr std::size_t SIZE = sizeof(Drawing::viewport_center) + sizeof(Drawing::viewport_factor);

        const vulkan::BufferMapper map(buffer_, OFFSET, SIZE);

        const decltype(Drawing().viewport_center) viewport_center = to_vector<float>(center);
        const decltype(Drawing().viewport_factor) viewport_factor = to_vector<float>(factor);

        map.write(0, viewport_center);
        map.write(sizeof(viewport_center), viewport_factor);
}

void DrawingBuffer::set_lighting_color(const Vector3f& color) const
{
        const decltype(Drawing().lighting_color) v = color;
        copy_to_buffer(offsetof(Drawing, lighting_color), v);
}

void DrawingBuffer::set_background_color(const Vector3f& color) const
{
        const decltype(Drawing().background_color) c = color;
        copy_to_buffer(offsetof(Drawing, background_color), c);
}

void DrawingBuffer::set_wireframe_color(const Vector3f& color) const
{
        const decltype(Drawing().wireframe_color) c = color;
        copy_to_buffer(offsetof(Drawing, wireframe_color), c);
}

void DrawingBuffer::set_clip_plane_color(const Vector3f& color) const
{
        const decltype(Drawing().clip_plane_color) c = color;
        copy_to_buffer(offsetof(Drawing, clip_plane_color), c);
}

void DrawingBuffer::set_normal_color_positive(const Vector3f& color) const
{
        const decltype(Drawing().normal_color_positive) c = color;
        copy_to_buffer(offsetof(Drawing, normal_color_positive), c);
}

void DrawingBuffer::set_normal_color_negative(const Vector3f& color) const
{
        const decltype(Drawing().normal_color_negative) c = color;
        copy_to_buffer(offsetof(Drawing, normal_color_negative), c);
}

void DrawingBuffer::set_normal_length(const float length) const
{
        const decltype(Drawing().normal_length) l = length;
        copy_to_buffer(offsetof(Drawing, normal_length), l);
}

void DrawingBuffer::set_show_materials(const bool show) const
{
        const decltype(Drawing().show_materials) s = show ? 1 : 0;
        copy_to_buffer(offsetof(Drawing, show_materials), s);
}

void DrawingBuffer::set_show_wireframe(const bool show) const
{
        const decltype(Drawing().show_wireframe) s = show ? 1 : 0;
        copy_to_buffer(offsetof(Drawing, show_wireframe), s);
}

void DrawingBuffer::set_show_shadow(const bool show) const
{
        const decltype(Drawing().show_shadow) s = show ? 1 : 0;
        copy_to_buffer(offsetof(Drawing, show_shadow), s);
}

void DrawingBuffer::set_show_fog(const bool show) const
{
        const decltype(Drawing().show_fog) s = show ? 1 : 0;
        copy_to_buffer(offsetof(Drawing, show_fog), s);
}

void DrawingBuffer::set_flat_shading(const bool flat_shading) const
{
        const decltype(Drawing().flat_shading) s = flat_shading ? 1 : 0;
        copy_to_buffer(offsetof(Drawing, flat_shading), s);
}

void DrawingBuffer::set_direction_to_light(const Vector3f& direction) const
{
        const decltype(Drawing().direction_to_light) d = direction;
        copy_to_buffer(offsetof(Drawing, direction_to_light), d);
}

void DrawingBuffer::set_lighting_proportions(const float front, const float side) const
{
        static_assert(
                offsetof(Drawing, front_lighting_proportion) + sizeof(Drawing::front_lighting_proportion)
                == offsetof(Drawing, side_lighting_proportion));

        constexpr std::size_t OFFSET = offsetof(Drawing, front_lighting_proportion);
        constexpr std::size_t SIZE =
                sizeof(Drawing::front_lighting_proportion) + sizeof(Drawing::side_lighting_proportion);

        const vulkan::BufferMapper map(buffer_, OFFSET, SIZE);

        const decltype(Drawing().front_lighting_proportion) front_lighting_proportion = front;
        const decltype(Drawing().side_lighting_proportion) side_lighting_proportion = side;

        map.write(0, front_lighting_proportion);
        map.write(sizeof(front_lighting_proportion), side_lighting_proportion);
}

void DrawingBuffer::set_direction_to_camera(const Vector3f& direction) const
{
        const decltype(Drawing().direction_to_camera) d = direction;
        copy_to_buffer(offsetof(Drawing, direction_to_camera), d);
}
}
