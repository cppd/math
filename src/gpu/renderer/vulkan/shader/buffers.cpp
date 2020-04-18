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

namespace gpu
{
RendererBuffers::RendererBuffers(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices)
{
        m_uniform_buffers.emplace_back(
                vulkan::BufferMemoryType::HostVisible, device, family_indices, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Matrices));
        m_matrices_buffer_index = m_uniform_buffers.size() - 1;

        m_uniform_buffers.emplace_back(
                vulkan::BufferMemoryType::HostVisible, device, family_indices, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Lighting));
        m_lighting_buffer_index = m_uniform_buffers.size() - 1;

        m_uniform_buffers.emplace_back(
                vulkan::BufferMemoryType::HostVisible, device, family_indices, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Drawing));
        m_drawing_buffer_index = m_uniform_buffers.size() - 1;
}

VkBuffer RendererBuffers::matrices_buffer() const
{
        return m_uniform_buffers[m_matrices_buffer_index];
}

VkDeviceSize RendererBuffers::matrices_size() const
{
        return m_uniform_buffers[m_matrices_buffer_index].size();
}

VkBuffer RendererBuffers::lighting_buffer() const
{
        return m_uniform_buffers[m_lighting_buffer_index];
}

VkDeviceSize RendererBuffers::lighting_size() const
{
        return m_uniform_buffers[m_lighting_buffer_index].size();
}

VkBuffer RendererBuffers::drawing_buffer() const
{
        return m_uniform_buffers[m_drawing_buffer_index];
}

VkDeviceSize RendererBuffers::drawing_size() const
{
        return m_uniform_buffers[m_drawing_buffer_index].size();
}

template <typename T>
void RendererBuffers::copy_to_matrices_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_matrices_buffer_index], offset, data);
}

template <typename T>
void RendererBuffers::copy_to_lighting_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_lighting_buffer_index], offset, data);
}

template <typename T>
void RendererBuffers::copy_to_drawing_buffer(VkDeviceSize offset, const T& data) const
{
        vulkan::map_and_write_to_buffer(m_uniform_buffers[m_drawing_buffer_index], offset, data);
}

void RendererBuffers::set_matrices(
        const mat4& main_mvp_matrix,
        const mat4& main_model_matrix,
        const mat4& main_vp_matrix,
        const mat4& shadow_mvp_matrix,
        const mat4& shadow_mvp_texture_matrix) const
{
        Matrices matrices;
        matrices.main_mvp_matrix = to_matrix<float>(main_mvp_matrix).transpose();
        matrices.main_model_matrix = to_matrix<float>(main_model_matrix).transpose();
        matrices.main_vp_matrix = to_matrix<float>(main_vp_matrix).transpose();
        matrices.shadow_mvp_matrix = to_matrix<float>(shadow_mvp_matrix).transpose();
        matrices.shadow_mvp_texture_matrix = to_matrix<float>(shadow_mvp_texture_matrix).transpose();
        copy_to_matrices_buffer(0, matrices);
}

void RendererBuffers::set_clip_plane(const vec4& equation, bool enabled) const
{
        static_assert(
                offsetof(Drawing, clip_plane_equation) + sizeof(Drawing::clip_plane_equation) ==
                offsetof(Drawing, clip_plane_enabled));

        constexpr size_t offset = offsetof(Drawing, clip_plane_equation);
        constexpr size_t size = sizeof(Drawing::clip_plane_equation) + sizeof(Drawing::clip_plane_enabled);

        vulkan::BufferMapper map(m_uniform_buffers[m_drawing_buffer_index], offset, size);

        decltype(Drawing().clip_plane_equation) clip_plane_equation = to_vector<float>(equation);
        decltype(Drawing().clip_plane_enabled) clip_plane_enabled = enabled ? 1 : 0;

        map.write(0, clip_plane_equation);
        map.write(sizeof(clip_plane_equation), clip_plane_enabled);
}

void RendererBuffers::set_default_color(const Color& color) const
{
        decltype(Drawing().default_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, default_color), c);
}

void RendererBuffers::set_wireframe_color(const Color& color) const
{
        decltype(Drawing().wireframe_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, wireframe_color), c);
}

void RendererBuffers::set_background_color(const Color& color) const
{
        decltype(Drawing().background_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, background_color), c);
}

void RendererBuffers::set_clip_plane_color(const Color& color) const
{
        decltype(Drawing().clip_plane_color) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, clip_plane_color), c);
}

void RendererBuffers::set_normal_length(float length) const
{
        decltype(Drawing().normal_length) l = length;
        copy_to_drawing_buffer(offsetof(Drawing, normal_length), l);
}

void RendererBuffers::set_normal_color_positive(const Color& color) const
{
        decltype(Drawing().normal_color_positive) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, normal_color_positive), c);
}

void RendererBuffers::set_normal_color_negative(const Color& color) const
{
        decltype(Drawing().normal_color_negative) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, normal_color_negative), c);
}

void RendererBuffers::set_default_ns(float default_ns) const
{
        decltype(Drawing().default_ns) ns = default_ns;
        copy_to_drawing_buffer(offsetof(Drawing, default_ns), ns);
}

void RendererBuffers::set_light_a(const Color& color) const
{
        decltype(Drawing().light_a) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_a), c);
}

void RendererBuffers::set_light_d(const Color& color) const
{
        decltype(Drawing().light_d) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_d), c);
}

void RendererBuffers::set_light_s(const Color& color) const
{
        decltype(Drawing().light_s) c = color.to_rgb_vector<float>();
        copy_to_drawing_buffer(offsetof(Drawing, light_s), c);
}

void RendererBuffers::set_show_materials(bool show) const
{
        decltype(Drawing().show_materials) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_materials), s);
}

void RendererBuffers::set_show_wireframe(bool show) const
{
        decltype(Drawing().show_wireframe) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_wireframe), s);
}

void RendererBuffers::set_show_shadow(bool show) const
{
        decltype(Drawing().show_shadow) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_shadow), s);
}

void RendererBuffers::set_show_fog(bool show) const
{
        decltype(Drawing().show_fog) s = show ? 1 : 0;
        copy_to_drawing_buffer(offsetof(Drawing, show_fog), s);
}

void RendererBuffers::set_direction_to_light(const vec3f& direction) const
{
        decltype(Lighting().direction_to_light) d = direction;
        copy_to_lighting_buffer(offsetof(Lighting, direction_to_light), d);
}

void RendererBuffers::set_direction_to_camera(const vec3f& direction) const
{
        decltype(Lighting().direction_to_camera) d = direction;
        copy_to_lighting_buffer(offsetof(Lighting, direction_to_camera), d);
}

void RendererBuffers::set_show_smooth(bool show) const
{
        decltype(Lighting().show_smooth) s = show ? 1 : 0;
        copy_to_lighting_buffer(offsetof(Lighting, show_smooth), s);
}

//

MaterialBuffer::MaterialBuffer(
        const vulkan::Device& device,
        const std::unordered_set<uint32_t>& family_indices,
        const Material& material)
        : m_uniform_buffer(device, family_indices, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Material), material)
{
}

VkBuffer MaterialBuffer::buffer() const
{
        return m_uniform_buffer;
}

VkDeviceSize MaterialBuffer::buffer_size() const
{
        return m_uniform_buffer.size();
}
}
