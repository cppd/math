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

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/vulkan/buffers.h>

#include <vector>

namespace ns::gpu::renderer
{
class DrawingBuffer final
{
        vulkan::BufferWithMemory buffer_;

        // If structures are placed in one buffer then
        // VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment
        // is the minimum required alignment for VkDescriptorBufferInfo::offset

        struct Drawing final
        {
                alignas(sizeof(Vector4f)) Matrix4f vp_matrix;
                alignas(sizeof(Vector4f)) Vector3f lighting_color;
                alignas(sizeof(Vector4f)) Vector3f background_color;
                alignas(sizeof(Vector4f)) Vector3f wireframe_color;
                alignas(sizeof(Vector4f)) Vector3f normal_color_positive;
                alignas(sizeof(Vector4f)) Vector3f normal_color_negative;
                float normal_length;
                std::uint32_t show_materials;
                std::uint32_t show_wireframe;
                std::uint32_t show_shadow;
                std::uint32_t show_fog;
                std::uint32_t show_smooth;
                alignas(sizeof(Vector4f)) Vector3f clip_plane_color;
                alignas(sizeof(Vector4f)) Vector4f clip_plane_equation;
                std::uint32_t clip_plane_enabled;
                alignas(sizeof(Vector4f)) Vector3f direction_to_light;
                alignas(sizeof(Vector4f)) Vector3f direction_to_camera;
                alignas(sizeof(Vector2f)) Vector2f viewport_center;
                alignas(sizeof(Vector2f)) Vector2f viewport_factor;
                std::uint32_t transparency_max_node_count;
        };

        template <typename T>
        void copy_to_buffer(VkDeviceSize offset, const T& data) const;

public:
        DrawingBuffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices);

        const vulkan::Buffer& buffer() const;

        void set_matrix(const Matrix4d& vp_matrix) const;

        void set_transparency_max_node_count(std::uint32_t count) const;

        void set_lighting_color(const Vector3f& color) const;
        void set_background_color(const Vector3f& color) const;
        void set_wireframe_color(const Vector3f& color) const;
        void set_clip_plane(const Vector4d& equation, bool enabled) const;
        void set_clip_plane_color(const Vector3f& color) const;
        void set_viewport(const Vector2d& center, const Vector2d& factor) const;
        void set_normal_color_positive(const Vector3f& color) const;
        void set_normal_color_negative(const Vector3f& color) const;
        void set_normal_length(float length) const;
        void set_show_materials(bool show) const;
        void set_direction_to_light(const Vector3f& direction) const;
        void set_direction_to_camera(const Vector3f& direction) const;
        void set_show_smooth(bool show) const;
        void set_show_wireframe(bool show) const;
        void set_show_shadow(bool show) const;
        void set_show_fog(bool show) const;
};
}
