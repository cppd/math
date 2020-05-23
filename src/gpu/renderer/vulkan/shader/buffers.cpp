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

#include "buffers.h"

namespace gpu::renderer
{
ShaderBuffers::ShaderBuffers(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices)
{
        m_uniform_buffers.emplace_back(
                vulkan::BufferMemoryType::HostVisible, device, family_indices, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Matrices));
        m_matrices_buffer_index = m_uniform_buffers.size() - 1;

        m_uniform_buffers.emplace_back(
                vulkan::BufferMemoryType::HostVisible, device, family_indices, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Matrices));
        m_shadow_matrices_buffer_index = m_uniform_buffers.size() - 1;

        m_uniform_buffers.emplace_back(
                vulkan::BufferMemoryType::HostVisible, device, family_indices, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Drawing));
        m_drawing_buffer_index = m_uniform_buffers.size() - 1;
}

const vulkan::Buffer& ShaderBuffers::matrices_buffer() const
{
        return m_uniform_buffers[m_matrices_buffer_index].buffer();
}

const vulkan::Buffer& ShaderBuffers::shadow_matrices_buffer() const
{
        return m_uniform_buffers[m_shadow_matrices_buffer_index].buffer();
}

const vulkan::Buffer& ShaderBuffers::drawing_buffer() const
{
        return m_uniform_buffers[m_drawing_buffer_index].buffer();
}

template <typename T>
void ShaderBuffers::copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_matrices_buffer_index], offset, data);
}

template <typename T>
void ShaderBuffers::copy_to_shadow_matrices_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_shadow_matrices_buffer_index], offset, data);
}

template <typename T>
void ShaderBuffers::copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_drawing_buffer_index], offset, data);
}

void ShaderBuffers::set_matrices(
        const mat4& main_model_matrix,
        const mat4& main_mvp_matrix,
        const mat4& main_vp_matrix,
        const mat4& shadow_mvp_matrix,
        const mat4& shadow_vp_matrix,
        const mat4& shadow_mvp_texture_matrix) const
{
        {
                Matrices matrices;
                matrices.main_mvp_matrix = to_matrix<float>(main_mvp_matrix).transpose();
                matrices.main_model_matrix = to_matrix<float>(main_model_matrix).transpose();
                matrices.main_vp_matrix = to_matrix<float>(main_vp_matrix).transpose();
                matrices.shadow_mvp_texture_matrix = to_matrix<float>(shadow_mvp_texture_matrix).transpose();
                copy_to_matrices_buffer(0, matrices);
        }
        {
                Matrices matrices;
                matrices.main_mvp_matrix = to_matrix<float>(shadow_mvp_matrix).transpose();
                matrices.main_model_matrix = to_matrix<float>(main_model_matrix).transpose();
                matrices.main_vp_matrix = to_matrix<float>(shadow_vp_matrix).transpose();
                matrices.shadow_mvp_texture_matrix = to_matrix<float>(shadow_mvp_texture_matrix).transpose();
                copy_to_shadow_matrices_buffer(0, matrices);
        }
}

void ShaderBuffers::set_clip_plane(const vec4& equation, bool enabled) const
{
        static_assert(
                offsetof(Drawing, clip_plane_equation) + sizeof(Drawing::clip_plane_equation)
                == offsetof(Drawing, clip_plane_enabled));

        constexpr size_t offset = offsetof(Drawing, clip_plane_equation);
        constexpr size_t size = sizeof(Drawing::clip_plane_equation) + sizeof(Drawing::clip_plane_enabled);

        vulkan::BufferMapper map(m_uniform_buffers[m_drawing_buffer_index], offset, size);

        decltype(Drawing().clip_plane_equation) clip_plane_equation = to_vector<float>(equation);
        decltype(Drawing().clip_plane_enabled) clip_plane_enabled = enabled ? 1 : 0;

        map.write(0, clip_plane_equation);
        map.write(sizeof(clip_plane_equation), clip_plane_enabled);
}

void ShaderBuffers::set_viewport(const vec2& center, const vec2& factor) const
{
        static_assert(
                offsetof(Drawing, viewport_center) + sizeof(Drawing::viewport_factor)
                == offsetof(Drawing, viewport_factor));

        constexpr size_t offset = offsetof(Drawing, viewport_center);
        constexpr size_t size = sizeof(Drawing::viewport_center) + sizeof(Drawing::viewport_factor);

        vulkan::BufferMapper map(m_uniform_buffers[m_drawing_buffer_index], offset, size);

        decltype(Drawing().viewport_center) viewport_center = to_vector<float>(center);
        decltype(Drawing().viewport_factor) viewport_factor = to_vector<float>(factor);

        map.write(0, viewport_center);
        map.write(sizeof(viewport_center), viewport_factor);
}

void ShaderBuffers::set_default_color(const Color& color) const
{
        decltype(Drawing().default_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, default_color), c);
}

void ShaderBuffers::set_wireframe_color(const Color& color) const
{
        decltype(Drawing().wireframe_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, wireframe_color), c);
}

void ShaderBuffers::set_background_color(const Color& color) const
{
        decltype(Drawing().background_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, background_color), c);
}

void ShaderBuffers::set_clip_plane_color(const Color& color) const
{
        decltype(Drawing().clip_plane_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, clip_plane_color), c);
}

void ShaderBuffers::set_normal_length(float length) const
{
        decltype(Drawing().normal_length) l = length;
        copy_to_drawing_buffer(offsetof(Drawing, normal_length), l);
}

void ShaderBuffers::set_normal_color_positive(const Color& color) const
{
        decltype(Drawing().normal_color_positive) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, normal_color_positive), c);
}

void ShaderBuffers::set_normal_color_negative(const Color& color) const
{
        decltype(Drawing().normal_color_negative) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, normal_color_negative), c);
}

void ShaderBuffers::set_default_ns(float default_ns) const
{
        decltype(Drawing().default_ns) ns = default_ns;
        copy_to_drawing_buffer(offsetof(Drawing, default_ns), ns);
}

void ShaderBuffers::set_light_a(const Color& color) const
{
        decltype(Drawing().light_a) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_a), c);
}

void ShaderBuffers::set_light_d(const Color& color) const
{
        decltype(Drawing().light_d) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_d), c);
}

void ShaderBuffers::set_light_s(const Color& color) const
{
        decltype(Drawing().light_s) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_s), c);
}

void ShaderBuffers::set_show_materials(bool show) const
{
        decltype(Drawing().show_materials) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_materials), s);
}

void ShaderBuffers::set_show_wireframe(bool show) const
{
        decltype(Drawing().show_wireframe) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_wireframe), s);
}

void ShaderBuffers::set_show_shadow(bool show) const
{
        decltype(Drawing().show_shadow) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_shadow), s);
}

void ShaderBuffers::set_show_fog(bool show) const
{
        decltype(Drawing().show_fog) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_fog), s);
}

void ShaderBuffers::set_show_smooth(bool show) const
{
        decltype(Drawing().show_smooth) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_smooth), s);
}

void ShaderBuffers::set_direction_to_light(const vec3f& direction) const
{
        decltype(Drawing().direction_to_light) d = direction;
        copy_to_drawing_buffer(offsetof(Drawing, direction_to_light), d);
}

void ShaderBuffers::set_direction_to_camera(const vec3f& direction) const
{
        decltype(Drawing().direction_to_camera) d = direction;
        copy_to_drawing_buffer(offsetof(Drawing, direction_to_camera), d);
}

//

MaterialBuffer::MaterialBuffer(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::unordered_set<uint32_t>& family_indices,
        const Material& material)
        : m_uniform_buffer(
                vulkan::BufferMemoryType::DeviceLocal,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Material))
{
        m_uniform_buffer.write(command_pool, queue, data_size(material), data_pointer(material));
}

VkBuffer MaterialBuffer::buffer() const
{
        return m_uniform_buffer;
}

VkDeviceSize MaterialBuffer::buffer_size() const
{
        return m_uniform_buffer.size();
}

//

VolumeBuffer::VolumeBuffer(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices)
        : m_uniform_buffer_coordinates(
                vulkan::BufferMemoryType::HostVisible,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Coordinates)),
          m_uniform_buffer_volume(
                  vulkan::BufferMemoryType::DeviceLocal,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  sizeof(Volume))
{
}

VkBuffer VolumeBuffer::buffer_coordinates() const
{
        return m_uniform_buffer_coordinates;
}

VkDeviceSize VolumeBuffer::buffer_coordinates_size() const
{
        return m_uniform_buffer_coordinates.size();
}

VkBuffer VolumeBuffer::buffer_volume() const
{
        return m_uniform_buffer_volume;
}

VkDeviceSize VolumeBuffer::buffer_volume_size() const
{
        return m_uniform_buffer_volume.size();
}

void VolumeBuffer::set_matrix_and_clip_plane(const mat4& inverse_mvp_matrix, const vec4& clip_plane_equation) const
{
        Coordinates coordinates;
        coordinates.inverse_mvp_matrix = to_matrix<float>(inverse_mvp_matrix).transpose();
        coordinates.clip_plane_equation = to_vector<float>(clip_plane_equation);
        vulkan::map_and_write_to_buffer(m_uniform_buffer_coordinates, 0, coordinates);
}

void VolumeBuffer::set_clip_plane(const vec4& clip_plane_equation) const
{
        decltype(Coordinates().clip_plane_equation) clip_plane = to_vector<float>(clip_plane_equation);
        vulkan::map_and_write_to_buffer(
                m_uniform_buffer_coordinates, offsetof(Coordinates, clip_plane_equation), clip_plane);
}

void VolumeBuffer::set_parameters(
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        float window_offset,
        float window_scale,
        float transparency,
        bool isosurface,
        float isovalue) const
{
        ASSERT(window_offset >= 0);
        ASSERT(window_scale > 0);
        ASSERT(transparency >= 0);
        ASSERT(isovalue >= 0 && isovalue <= 1);

        Volume::Parameters parameters;

        parameters.window_offset = window_offset;
        parameters.window_scale = window_scale;
        parameters.transparency = transparency;
        parameters.isosurface = isosurface ? 1 : 0;
        parameters.isovalue = isovalue;

        m_uniform_buffer_volume.write(
                command_pool, queue, offsetof(Volume, parameters), data_size(parameters), data_pointer(parameters));
}

void VolumeBuffer::set_color_volume(
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        bool color_volume) const
{
        decltype(Volume().color_volume) v = color_volume ? 1 : 0;
        m_uniform_buffer_volume.write(
                command_pool, queue, offsetof(Volume, color_volume), data_size(v), data_pointer(v));
}
}
