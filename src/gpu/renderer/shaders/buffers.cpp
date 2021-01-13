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

#include "buffers.h"

#include "../commands.h"

namespace ns::gpu::renderer
{
namespace
{
template <typename Dst, typename Src>
Matrix<4, 4, Dst> mat4_std140(const Matrix<4, 4, Src>& m)
{
        Matrix<4, 4, Dst> res;
        for (unsigned r = 0; r < 4; ++r)
        {
                for (unsigned c = 0; c < 4; ++c)
                {
                        res(c, r) = m(r, c);
                }
        }
        return res;
}

template <typename Dst, typename Src>
Matrix<3, 4, Dst> mat3_std140(const Matrix<3, 3, Src>& m)
{
        Matrix<3, 4, Dst> res;
        for (unsigned r = 0; r < 3; ++r)
        {
                for (unsigned c = 0; c < 3; ++c)
                {
                        res(c, r) = m(r, c);
                }
        }
        return res;
}
}

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
        const mat4& main_vp_matrix,
        const mat4& shadow_vp_matrix,
        const mat4& shadow_vp_texture_matrix) const
{
        {
                Matrices matrices;
                matrices.vp_matrix = mat4_std140<float>(main_vp_matrix);
                matrices.shadow_vp_texture_matrix = mat4_std140<float>(shadow_vp_texture_matrix);
                copy_to_matrices_buffer(0, matrices);
        }
        {
                Matrices matrices;
                matrices.vp_matrix = mat4_std140<float>(shadow_vp_matrix);
                matrices.shadow_vp_texture_matrix = mat4_std140<float>(shadow_vp_texture_matrix);
                copy_to_shadow_matrices_buffer(0, matrices);
        }
}

void ShaderBuffers::set_transparency_max_node_count(uint32_t count) const
{
        decltype(Drawing().transparency_max_node_count) c = count;
        copy_to_drawing_buffer(offsetof(Drawing, transparency_max_node_count), c);
}

void ShaderBuffers::set_clip_plane(const vec4& equation, bool enabled) const
{
        static_assert(
                offsetof(Drawing, clip_plane_equation) + sizeof(Drawing::clip_plane_equation)
                == offsetof(Drawing, clip_plane_enabled));

        constexpr std::size_t offset = offsetof(Drawing, clip_plane_equation);
        constexpr std::size_t size = sizeof(Drawing::clip_plane_equation) + sizeof(Drawing::clip_plane_enabled);

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

        constexpr std::size_t offset = offsetof(Drawing, viewport_center);
        constexpr std::size_t size = sizeof(Drawing::viewport_center) + sizeof(Drawing::viewport_factor);

        vulkan::BufferMapper map(m_uniform_buffers[m_drawing_buffer_index], offset, size);

        decltype(Drawing().viewport_center) viewport_center = to_vector<float>(center);
        decltype(Drawing().viewport_factor) viewport_factor = to_vector<float>(factor);

        map.write(0, viewport_center);
        map.write(sizeof(viewport_center), viewport_factor);
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

void ShaderBuffers::set_lighting_intensity(float intensity) const
{
        decltype(Drawing().lighting_intensity) v = intensity;
        copy_to_drawing_buffer(offsetof(Drawing, lighting_intensity), v);
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
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                sizeof(Material))
{
        m_uniform_buffer.write(command_pool, queue, data_size(material), data_pointer(material));
}

const vulkan::Buffer& MaterialBuffer::buffer() const
{
        return m_uniform_buffer.buffer();
}

//

MeshBuffer::MeshBuffer(const vulkan::Device& device, const std::unordered_set<uint32_t>& family_indices)
        : m_uniform_buffer(
                vulkan::BufferMemoryType::HostVisible,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Mesh))
{
}

const vulkan::Buffer& MeshBuffer::buffer() const
{
        return m_uniform_buffer.buffer();
}

void MeshBuffer::set_coordinates(const mat4& model_matrix, const mat3& normal_matrix) const
{
        static_assert(offsetof(Mesh, model_matrix) + sizeof(Mesh::model_matrix) == offsetof(Mesh, normal_matrix));

        constexpr std::size_t offset = offsetof(Mesh, model_matrix);
        constexpr std::size_t size = offsetof(Mesh, normal_matrix) + sizeof(Mesh::normal_matrix) - offset;

        vulkan::BufferMapper map(m_uniform_buffer, offset, size);

        decltype(Mesh().model_matrix) model = mat4_std140<float>(model_matrix);
        decltype(Mesh().normal_matrix) normal = mat3_std140<float>(normal_matrix);

        map.write(offsetof(Mesh, model_matrix) - offset, model);
        map.write(offsetof(Mesh, normal_matrix) - offset, normal);
}

void MeshBuffer::set_color(const Color& color) const
{
        decltype(Mesh().color) c = color.to_rgb_vector<float>();
        vulkan::map_and_write_to_buffer(m_uniform_buffer, offsetof(Mesh, color), c);
}

void MeshBuffer::set_alpha(float alpha) const
{
        decltype(Mesh().alpha) a = alpha;
        vulkan::map_and_write_to_buffer(m_uniform_buffer, offsetof(Mesh, alpha), a);
}

void MeshBuffer::set_lighting(float ambient, float metalness, float roughness) const
{
        static_assert(offsetof(Mesh, roughness) - offsetof(Mesh, ambient) == 2 * sizeof(float));

        constexpr std::size_t offset = offsetof(Mesh, ambient);
        constexpr std::size_t size = offsetof(Mesh, roughness) + sizeof(Mesh::roughness) - offset;

        vulkan::BufferMapper map(m_uniform_buffer, offset, size);

        Mesh mesh;
        mesh.ambient = ambient;
        mesh.metalness = metalness;
        mesh.roughness = roughness;

        map.write(0, size, reinterpret_cast<const char*>(&mesh) + offset);
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
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
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

void VolumeBuffer::set_coordinates(
        const mat4& inverse_mvp_matrix,
        const vec4& third_row_of_mvp,
        const vec4& clip_plane_equation,
        const vec3& gradient_h,
        const mat3& normal_matrix) const
{
        Coordinates coordinates;
        coordinates.inverse_mvp_matrix = mat4_std140<float>(inverse_mvp_matrix);
        coordinates.third_row_of_mvp = to_vector<float>(third_row_of_mvp);
        coordinates.clip_plane_equation = to_vector<float>(clip_plane_equation);
        coordinates.gradient_h = to_vector<float>(gradient_h);
        coordinates.normal_matrix = mat3_std140<float>(normal_matrix);
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
        float volume_alpha_coefficient,
        float isosurface_alpha,
        bool isosurface,
        float isovalue,
        const Color& color) const
{
        ASSERT(window_offset >= 0);
        ASSERT(window_scale > 0);
        ASSERT(volume_alpha_coefficient >= 0);
        ASSERT(isosurface_alpha >= 0 && isosurface_alpha <= 1);
        ASSERT(isovalue >= 0 && isovalue <= 1);

        static_assert(offsetof(Volume, color) - offsetof(Volume, window_offset) == 8 * sizeof(float));

        constexpr std::size_t offset = offsetof(Volume, window_offset);
        constexpr std::size_t size = offsetof(Volume, color) + sizeof(Volume::color) - offset;

        Volume volume;

        volume.window_offset = window_offset;
        volume.window_scale = window_scale;
        volume.volume_alpha_coefficient = volume_alpha_coefficient;
        volume.isosurface_alpha = isosurface_alpha;
        volume.isosurface = isosurface ? 1 : 0;
        volume.isovalue = isovalue;
        volume.color = color.to_rgb_vector<float>();

        m_uniform_buffer_volume.write(
                command_pool, queue, offset, size, reinterpret_cast<const char*>(&volume) + offset);
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

void VolumeBuffer::set_lighting(
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        float ambient,
        float metalness,
        float roughness) const
{
        static_assert(offsetof(Volume, roughness) - offsetof(Volume, ambient) == 2 * sizeof(float));

        constexpr std::size_t offset = offsetof(Volume, ambient);
        constexpr std::size_t size = offsetof(Volume, roughness) + sizeof(Volume::roughness) - offset;

        Volume volume;

        volume.ambient = ambient;
        volume.metalness = metalness;
        volume.roughness = roughness;

        m_uniform_buffer_volume.write(
                command_pool, queue, offset, size, reinterpret_cast<const char*>(&volume) + offset);
}

TransparencyBuffers::TransparencyBuffers(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::unordered_set<uint32_t>& family_indices,
        VkSampleCountFlagBits sample_count,
        unsigned width,
        unsigned height,
        unsigned long long max_node_buffer_size)
        : m_node_count(
                std::min(
                        max_node_buffer_size,
                        static_cast<unsigned long long>(device.properties().properties_10.limits.maxStorageBufferRange))
                / NODE_SIZE),
          m_heads(device,
                  command_pool,
                  queue,
                  family_indices,
                  std::vector<VkFormat>({VK_FORMAT_R32_UINT}),
                  sample_count,
                  VK_IMAGE_TYPE_2D,
                  vulkan::make_extent(width, height),
                  VK_IMAGE_LAYOUT_GENERAL,
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT),
          m_heads_size(
                  device,
                  command_pool,
                  queue,
                  family_indices,
                  std::vector<VkFormat>({VK_FORMAT_R32_UINT}),
                  sample_count,
                  VK_IMAGE_TYPE_2D,
                  vulkan::make_extent(width, height),
                  VK_IMAGE_LAYOUT_GENERAL,
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT),
          m_node_buffer(
                  vulkan::BufferMemoryType::DeviceLocal,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                  m_node_count * NODE_SIZE),
          m_init_buffer(
                  vulkan::BufferMemoryType::HostVisible,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  sizeof(Counters)),
          m_read_buffer(
                  vulkan::BufferMemoryType::HostVisible,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  sizeof(Counters)),
          m_counters(
                  vulkan::BufferMemoryType::DeviceLocal,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                          | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                  sizeof(Counters))
{
        Counters counters;
        counters.transparency_node_counter = 0;
        counters.transparency_overload_counter = 0;
        vulkan::BufferMapper mapper(m_init_buffer, 0, m_init_buffer.size());
        mapper.write(counters);
}

const vulkan::Buffer& TransparencyBuffers::counters() const
{
        return m_counters.buffer();
}

const vulkan::ImageWithMemory& TransparencyBuffers::heads() const
{
        return m_heads;
}

const vulkan::ImageWithMemory& TransparencyBuffers::heads_size() const
{
        return m_heads_size;
}

const vulkan::Buffer& TransparencyBuffers::nodes() const
{
        return m_node_buffer.buffer();
}

unsigned TransparencyBuffers::node_count() const
{
        return m_node_count;
}

void TransparencyBuffers::commands_init(VkCommandBuffer command_buffer) const
{
        commands_init_uint32_storage_image(command_buffer, m_heads, HEADS_NULL_POINTER);
        commands_init_uint32_storage_image(command_buffer, m_heads_size, 0);
        commands_init_buffer(command_buffer, m_init_buffer, m_counters);
}

void TransparencyBuffers::commands_read(VkCommandBuffer command_buffer) const
{
        commands_read_buffer(command_buffer, m_counters, m_read_buffer);
}

void TransparencyBuffers::read(unsigned long long* required_node_memory, unsigned* overload_counter) const
{
        vulkan::BufferMapper mapper(m_read_buffer);
        Counters counters;
        mapper.read(&counters);
        *required_node_memory = counters.transparency_node_counter * NODE_SIZE;
        *overload_counter = counters.transparency_overload_counter;
}
}
