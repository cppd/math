/*
Copyright (C) 2017-2021 Topological Manifold

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
#include <src/numerical/vec.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/descriptor.h>

#include <vector>

namespace ns::gpu::renderer
{
class ShaderBuffers
{
        std::vector<vulkan::BufferWithMemory> uniform_buffers_;

        // If structures are placed in one buffer then
        // VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment
        // is the minimum required alignment for VkDescriptorBufferInfo::offset

        struct Matrices
        {
                mat4f vp_matrix;
                mat4f shadow_vp_texture_matrix;
        };

        struct Drawing
        {
                alignas(sizeof(vec4f)) vec3f lighting_color;
                alignas(sizeof(vec4f)) vec3f background_color;
                alignas(sizeof(vec4f)) vec3f wireframe_color;
                alignas(sizeof(vec4f)) vec3f normal_color_positive;
                alignas(sizeof(vec4f)) vec3f normal_color_negative;
                float normal_length;
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
                uint32_t transparency_max_node_count;
        };

        std::size_t matrices_buffer_index_;
        std::size_t shadow_matrices_buffer_index_;
        std::size_t drawing_buffer_index_;

        template <typename T>
        void copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const;
        template <typename T>
        void copy_to_shadow_matrices_buffer(VkDeviceSize offset, const T& data) const;
        template <typename T>
        void copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const;

public:
        ShaderBuffers(const vulkan::Device& device, const std::vector<uint32_t>& family_indices);

        const vulkan::Buffer& matrices_buffer() const;
        const vulkan::Buffer& shadow_matrices_buffer() const;
        const vulkan::Buffer& drawing_buffer() const;

        void set_matrices(
                const mat4d& main_vp_matrix,
                const mat4d& shadow_vp_matrix,
                const mat4d& shadow_vp_texture_matrix) const;

        void set_transparency_max_node_count(uint32_t count) const;

        void set_lighting_color(const vec3f& color) const;
        void set_background_color(const vec3f& color) const;
        void set_wireframe_color(const vec3f& color) const;
        void set_clip_plane(const vec4d& equation, bool enabled) const;
        void set_clip_plane_color(const vec3f& color) const;
        void set_viewport(const vec2d& center, const vec2d& factor) const;
        void set_normal_color_positive(const vec3f& color) const;
        void set_normal_color_negative(const vec3f& color) const;
        void set_normal_length(float length) const;
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
        vulkan::BufferWithMemory uniform_buffer_;

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
                const std::vector<uint32_t>& family_indices,
                const Material& material);

        const vulkan::Buffer& buffer() const;
};

class MeshBuffer final
{
        vulkan::BufferWithMemory uniform_buffer_;

        struct Mesh
        {
                alignas(sizeof(vec4f)) mat4f model_matrix;
                alignas(sizeof(vec4f)) Matrix<3, 4, float> normal_matrix;
                alignas(sizeof(vec4f)) vec3f color;
                float alpha;
                float ambient;
                float metalness;
                float roughness;
        };

public:
        MeshBuffer(const vulkan::Device& device, const std::vector<uint32_t>& family_indices);

        const vulkan::Buffer& buffer() const;

        void set_coordinates(const mat4d& model_matrix, const mat3d& normal_matrix) const;
        void set_color(const vec3f& color) const;
        void set_alpha(float alpha) const;
        void set_lighting(float ambient, float metalness, float roughness) const;
};

class VolumeBuffer final
{
        vulkan::BufferWithMemory uniform_buffer_coordinates_;
        vulkan::BufferWithMemory uniform_buffer_volume_;

        struct Coordinates
        {
                alignas(sizeof(vec4f)) mat4f inverse_mvp_matrix;
                alignas(sizeof(vec4f)) vec4f third_row_of_mvp;
                alignas(sizeof(vec4f)) vec4f clip_plane_equation;
                alignas(sizeof(vec4f)) vec3f gradient_h;
                alignas(sizeof(vec4f)) Matrix<3, 4, float> normal_matrix;
        };

        struct Volume
        {
                float window_offset;
                float window_scale;
                float volume_alpha_coefficient;
                float isosurface_alpha;
                uint32_t isosurface;
                float isovalue;
                alignas(sizeof(vec4f)) vec3f color;
                uint32_t color_volume;
                float ambient;
                float metalness;
                float roughness;
        };

public:
        VolumeBuffer(
                const vulkan::Device& device,
                const std::vector<uint32_t>& graphics_family_indices,
                const std::vector<uint32_t>& transfer_family_indices);

        VkBuffer buffer_coordinates() const;
        VkDeviceSize buffer_coordinates_size() const;

        VkBuffer buffer_volume() const;
        VkDeviceSize buffer_volume_size() const;

        void set_coordinates(
                const mat4d& inverse_mvp_matrix,
                const vec4d& third_row_of_mvp,
                const vec4d& clip_plane_equation,
                const vec3d& gradient_h,
                const mat3d& normal_matrix) const;

        void set_clip_plane(const vec4d& clip_plane_equation) const;

        void set_parameters(
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                float window_offset,
                float window_scale,
                float volume_alpha_coefficient,
                float isosurface_alpha,
                bool isosurface,
                float isovalue,
                const vec3f& color) const;

        void set_color_volume(const vulkan::CommandPool& command_pool, const vulkan::Queue& queue, bool color_volume)
                const;

        void set_lighting(
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                float ambient,
                float metalness,
                float roughness) const;
};

class TransparencyBuffers
{
        static constexpr uint32_t HEADS_NULL_POINTER = limits<uint32_t>::max();

        // (uint color_rg) + (uint color_ba) + (float depth) + (uint next)
        static constexpr uint32_t NODE_SIZE = 16;

        const unsigned node_count_;

        vulkan::ImageWithMemory heads_;
        vulkan::ImageWithMemory heads_size_;
        vulkan::BufferWithMemory node_buffer_;

        vulkan::BufferWithMemory init_buffer_;
        vulkan::BufferWithMemory read_buffer_;
        vulkan::BufferWithMemory counters_;

        struct Counters
        {
                uint32_t transparency_node_counter;
                uint32_t transparency_overload_counter;
        };

public:
        TransparencyBuffers(
                const vulkan::Device& device,
                const vulkan::CommandPool& command_pool,
                const vulkan::Queue& queue,
                const std::vector<uint32_t>& family_indices,
                VkSampleCountFlagBits sample_count,
                unsigned width,
                unsigned height,
                unsigned long long max_node_buffer_size);

        const vulkan::Buffer& counters() const;
        const vulkan::ImageWithMemory& heads() const;
        const vulkan::ImageWithMemory& heads_size() const;
        const vulkan::Buffer& nodes() const;

        unsigned node_count() const;

        void commands_init(VkCommandBuffer command_buffer) const;
        void commands_read(VkCommandBuffer command_buffer) const;

        void read(unsigned long long* required_node_memory, unsigned* overload_counter) const;
};
}
