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

#include <src/color/color.h>
#include <src/numerical/matrix.h>
#include <src/vulkan/buffers.h>

#include <unordered_set>
#include <vector>

namespace gpu::renderer
{
class ShaderBuffers
{
        std::vector<vulkan::BufferWithMemory> m_uniform_buffers;

        // Если размещать структуры в одном буфере, то требуется выравнивание каждой структуры
        // на VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment для VkDescriptorBufferInfo::offset

        struct Matrices
        {
                mat4f main_mvp_matrix;
                mat4f main_model_matrix;
                mat4f main_vp_matrix;
                mat4f shadow_mvp_texture_matrix;
        };

        struct Drawing
        {
                alignas(sizeof(vec4f)) vec3f default_color;
                alignas(sizeof(vec4f)) vec3f wireframe_color;
                alignas(sizeof(vec4f)) vec3f background_color;
                float normal_length;
                alignas(sizeof(vec4f)) vec3f normal_color_positive;
                alignas(sizeof(vec4f)) vec3f normal_color_negative;
                float default_ns;
                alignas(sizeof(vec4f)) vec3f light_a;
                alignas(sizeof(vec4f)) vec3f light_d;
                alignas(sizeof(vec4f)) vec3f light_s;
                uint32_t show_materials;
                uint32_t show_wireframe;
                uint32_t show_shadow;
                uint32_t show_fog;
                uint32_t show_smooth;
                alignas(sizeof(vec4f)) vec3f clip_plane_color;
                alignas(sizeof(vec4f)) vec4f clip_plane_equation;
                uint32_t clip_plane_enabled;
                alignas(sizeof(vec4f)) vec3f direction_to_light;
                alignas(sizeof(vec4f)) vec3f direction_to_camera;
                alignas(sizeof(vec2f)) vec2f viewport_center;
                alignas(sizeof(vec2f)) vec2f viewport_factor;
        };

        size_t m_matrices_buffer_index;
        size_t m_shadow_matrices_buffer_index;
        size_t m_drawing_buffer_index;

        template <typename T>
        void copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const;
        template <typename T>
        void copy_to_shadow_matrices_buffer(VkDeviceSize offset, const T& data) const;
        template <typename T>
        void copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const;

public:
        ShaderBuffers(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices);

        const vulkan::Buffer& matrices_buffer() const;
        const vulkan::Buffer& shadow_matrices_buffer() const;
        const vulkan::Buffer& drawing_buffer() const;

        void set_matrices(
                const mat4& main_model_matrix,
                const mat4& main_mvp_matrix,
                const mat4& main_vp_matrix,
                const mat4& shadow_mvp_matrix,
                const mat4& shadow_vp_matrix,
                const mat4& shadow_mvp_texture_matrix) const;

        void set_clip_plane(const vec4& equation, bool enabled) const;
        void set_viewport(const vec2& center, const vec2& factor) const;
        void set_default_color(const Color& color) const;
        void set_wireframe_color(const Color& color) const;
        void set_background_color(const Color& color) const;
        void set_clip_plane_color(const Color& color) const;
        void set_normal_length(float length) const;
        void set_normal_color_positive(const Color& color) const;
        void set_normal_color_negative(const Color& color) const;
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

class MaterialBuffer final
{
        vulkan::BufferWithMemory m_uniform_buffer;

public:
        struct Material
        {
                alignas(sizeof(vec4f)) vec3f Ka;
                alignas(sizeof(vec4f)) vec3f Kd;
                alignas(sizeof(vec4f)) vec3f Ks;
                float Ns;
                uint32_t use_texture_Ka;
                uint32_t use_texture_Kd;
                uint32_t use_texture_Ks;
                uint32_t use_material;
        };

        MaterialBuffer(
                const vulkan::Device& device,
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                const std::unordered_set<uint32_t>& family_indices,
                const Material& material);

        VkBuffer buffer() const;
        VkDeviceSize buffer_size() const;
};

struct MaterialInfo final
{
        VkBuffer buffer;
        VkDeviceSize buffer_size;
        VkImageView texture_Ka;
        VkImageView texture_Kd;
        VkImageView texture_Ks;
};

class VolumeBuffer final
{
        vulkan::BufferWithMemory m_uniform_buffer_coordinates;
        vulkan::BufferWithMemory m_uniform_buffer_volume;

        struct Coordinates
        {
                alignas(sizeof(vec4f)) mat4f inverse_mvp_matrix;
                alignas(sizeof(vec4f)) vec4f clip_plane_equation;
        };

        struct Volume
        {
                struct Window
                {
                        float offset;
                        float scale;
                };
                Window window;
                uint32_t color_volume;
        };

public:
        VolumeBuffer(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices);

        VkBuffer buffer_coordinates() const;
        VkDeviceSize buffer_coordinates_size() const;

        VkBuffer buffer_volume() const;
        VkDeviceSize buffer_volume_size() const;

        void set_matrix_and_clip_plane(const mat4& inverse_mvp_matrix, const vec4& clip_plane_equation) const;
        void set_clip_plane(const vec4& clip_plane_equation) const;

        void set_window(
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                float window_offset,
                float window_scale) const;

        void set_color_volume(const vulkan::CommandPool& command_pool, const vulkan::Queue& queue, bool color_volume)
                const;
};

struct VolumeInfo final
{
        VkBuffer buffer_coordinates;
        VkDeviceSize buffer_coordinates_size;
        VkBuffer buffer_volume;
        VkDeviceSize buffer_volume_size;
        VkImageView image;
        VkImageView transfer_function;
};
}
