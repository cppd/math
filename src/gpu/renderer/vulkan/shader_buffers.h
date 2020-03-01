/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "../../com/glsl.h"

#include <src/color/color.h>
#include <src/numerical/matrix.h>
#include <src/vulkan/buffers.h>

#include <unordered_set>
#include <vector>

namespace gpu_vulkan
{
class RendererBuffers
{
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

        // Если размещать структуры в одном буфере, то требуется выравнивание каждой структуры
        // на VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment для VkDescriptorBufferInfo::offset

        struct Matrices
        {
                struct M
                {
                        mat4f main_mvp_matrix;
                        mat4f main_model_matrix;
                        mat4f main_vp_matrix;
                        mat4f shadow_mvp_matrix;
                        mat4f shadow_mvp_texture_matrix;
                };
                struct C
                {
                        vec4f equation;
                        vec4f equation_shadow;
                        uint32_t enabled;
                };
                M matrices;
                C clip_plane;
        };

        struct Lighting
        {
                alignas(GLSL_VEC3_ALIGN) vec3f direction_to_light;
                alignas(GLSL_VEC3_ALIGN) vec3f direction_to_camera;
                uint32_t show_smooth;
        };

        struct Drawing
        {
                alignas(GLSL_VEC3_ALIGN) vec3f default_color;
                alignas(GLSL_VEC3_ALIGN) vec3f wireframe_color;
                alignas(GLSL_VEC3_ALIGN) vec3f background_color;
                alignas(GLSL_VEC3_ALIGN) vec3f clip_plane_color;
                float default_ns;
                alignas(GLSL_VEC3_ALIGN) vec3f light_a;
                alignas(GLSL_VEC3_ALIGN) vec3f light_d;
                alignas(GLSL_VEC3_ALIGN) vec3f light_s;
                uint32_t show_materials;
                uint32_t show_wireframe;
                uint32_t show_shadow;
                uint32_t show_fog;
        };

        size_t m_matrices_buffer_index;
        size_t m_lighting_buffer_index;
        size_t m_drawing_buffer_index;

        template <typename T>
        void copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const;
        template <typename T>
        void copy_to_lighting_buffer(VkDeviceSize offset, const T& data) const;
        template <typename T>
        void copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const;

public:
        RendererBuffers(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices);

        VkBuffer matrices_buffer() const;
        VkDeviceSize matrices_size() const;

        VkBuffer lighting_buffer() const;
        VkDeviceSize lighting_size() const;

        VkBuffer drawing_buffer() const;
        VkDeviceSize drawing_size() const;

        void set_matrices(
                const mat4& main_mvp_matrix,
                const mat4& main_model_matrix,
                const mat4& main_vp_matrix,
                const mat4& shadow_mvp_matrix,
                const mat4& shadow_mvp_texture_matrix) const;
        void set_clip_plane(const vec4& equation, const vec4& equation_shadow, bool enabled) const;
        void set_default_color(const Color& color) const;
        void set_wireframe_color(const Color& color) const;
        void set_background_color(const Color& color) const;
        void set_clip_plane_color(const Color& color) const;
        void set_default_ns(float default_ns) const;
        void set_light_a(const Color& color) const;
        void set_light_d(const Color& color) const;
        void set_light_s(const Color& color) const;
        void set_show_materials(bool show) const;
        void set_direction_to_light(const vec3f& direction) const;
        void set_direction_to_camera(const vec3f& direction) const;
        void set_show_smooth(bool show) const;
        void set_show_wireframe(bool show) const;
        void set_show_shadow(bool show) const;
        void set_show_fog(bool show) const;
};
}
