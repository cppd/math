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

#include "../../com/matrix.h"
#include "../buffer_commands.h"

#include <src/com/merge.h>

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

//

MaterialBuffer::MaterialBuffer(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const Material& material)
        : uniform_buffer_(
                vulkan::BufferMemoryType::DEVICE_LOCAL,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                sizeof(Material))
{
        uniform_buffer_.write(command_pool, queue, data_size(material), data_pointer(material));
}

const vulkan::Buffer& MaterialBuffer::buffer() const
{
        return uniform_buffer_.buffer();
}

//

MeshBuffer::MeshBuffer(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices)
        : uniform_buffer_(
                vulkan::BufferMemoryType::HOST_VISIBLE,
                device,
                family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Mesh))
{
}

const vulkan::Buffer& MeshBuffer::buffer() const
{
        return uniform_buffer_.buffer();
}

void MeshBuffer::set_coordinates(const Matrix4d& model_matrix, const Matrix3d& normal_matrix) const
{
        static_assert(offsetof(Mesh, model_matrix) + sizeof(Mesh::model_matrix) == offsetof(Mesh, normal_matrix));

        constexpr std::size_t OFFSET = offsetof(Mesh, model_matrix);
        constexpr std::size_t SIZE = offsetof(Mesh, normal_matrix) + sizeof(Mesh::normal_matrix) - OFFSET;

        vulkan::BufferMapper map(uniform_buffer_, OFFSET, SIZE);

        decltype(Mesh().model_matrix) model = to_std140<float>(model_matrix);
        decltype(Mesh().normal_matrix) normal = to_std140<float>(normal_matrix);

        map.write(offsetof(Mesh, model_matrix) - OFFSET, model);
        map.write(offsetof(Mesh, normal_matrix) - OFFSET, normal);
}

void MeshBuffer::set_color(const Vector3f& color) const
{
        decltype(Mesh().color) c = color;
        vulkan::map_and_write_to_buffer(uniform_buffer_, offsetof(Mesh, color), c);
}

void MeshBuffer::set_alpha(const float alpha) const
{
        decltype(Mesh().alpha) a = alpha;
        vulkan::map_and_write_to_buffer(uniform_buffer_, offsetof(Mesh, alpha), a);
}

void MeshBuffer::set_lighting(const float ambient, const float metalness, const float roughness) const
{
        static_assert(offsetof(Mesh, roughness) - offsetof(Mesh, ambient) == 2 * sizeof(float));

        constexpr std::size_t OFFSET = offsetof(Mesh, ambient);
        constexpr std::size_t SIZE = offsetof(Mesh, roughness) + sizeof(Mesh::roughness) - OFFSET;

        vulkan::BufferMapper map(uniform_buffer_, OFFSET, SIZE);

        Mesh mesh;
        mesh.ambient = ambient;
        mesh.metalness = metalness;
        mesh.roughness = roughness;

        map.write(0, SIZE, reinterpret_cast<const char*>(&mesh) + OFFSET);
}

//

VolumeBuffer::VolumeBuffer(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& graphics_family_indices,
        const std::vector<std::uint32_t>& transfer_family_indices)
        : uniform_buffer_coordinates_(
                vulkan::BufferMemoryType::HOST_VISIBLE,
                device,
                graphics_family_indices,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                sizeof(Coordinates)),
          uniform_buffer_volume_(
                  vulkan::BufferMemoryType::DEVICE_LOCAL,
                  device,
                  merge<std::vector<std::uint32_t>>(graphics_family_indices, transfer_family_indices),
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  sizeof(Volume))
{
}

VkBuffer VolumeBuffer::buffer_coordinates() const
{
        return uniform_buffer_coordinates_.buffer();
}

VkDeviceSize VolumeBuffer::buffer_coordinates_size() const
{
        return uniform_buffer_coordinates_.buffer().size();
}

VkBuffer VolumeBuffer::buffer_volume() const
{
        return uniform_buffer_volume_.buffer();
}

VkDeviceSize VolumeBuffer::buffer_volume_size() const
{
        return uniform_buffer_volume_.buffer().size();
}

void VolumeBuffer::set_coordinates(
        const Matrix4d& inverse_mvp_matrix,
        const Vector4d& third_row_of_mvp,
        const Vector4d& clip_plane_equation,
        const Vector3d& gradient_h,
        const Matrix3d& normal_matrix) const
{
        Coordinates coordinates;
        coordinates.inverse_mvp_matrix = to_std140<float>(inverse_mvp_matrix);
        coordinates.third_row_of_mvp = to_vector<float>(third_row_of_mvp);
        coordinates.clip_plane_equation = to_vector<float>(clip_plane_equation);
        coordinates.gradient_h = to_vector<float>(gradient_h);
        coordinates.normal_matrix = to_std140<float>(normal_matrix);
        vulkan::map_and_write_to_buffer(uniform_buffer_coordinates_, 0, coordinates);
}

void VolumeBuffer::set_clip_plane(const Vector4d& clip_plane_equation) const
{
        decltype(Coordinates().clip_plane_equation) clip_plane = to_vector<float>(clip_plane_equation);
        vulkan::map_and_write_to_buffer(
                uniform_buffer_coordinates_, offsetof(Coordinates, clip_plane_equation), clip_plane);
}

void VolumeBuffer::set_parameters(
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const float window_offset,
        const float window_scale,
        const float volume_alpha_coefficient,
        const float isosurface_alpha,
        const bool isosurface,
        const float isovalue,
        const Vector3f& color) const
{
        ASSERT(window_offset >= 0);
        ASSERT(window_scale > 0);
        ASSERT(volume_alpha_coefficient >= 0);
        ASSERT(isosurface_alpha >= 0 && isosurface_alpha <= 1);
        ASSERT(isovalue >= 0 && isovalue <= 1);

        static_assert(offsetof(Volume, color) - offsetof(Volume, window_offset) == 8 * sizeof(float));

        constexpr std::size_t OFFSET = offsetof(Volume, window_offset);
        constexpr std::size_t SIZE = offsetof(Volume, color) + sizeof(Volume::color) - OFFSET;

        Volume volume;

        volume.window_offset = window_offset;
        volume.window_scale = window_scale;
        volume.volume_alpha_coefficient = volume_alpha_coefficient;
        volume.isosurface_alpha = isosurface_alpha;
        volume.isosurface = isosurface ? 1 : 0;
        volume.isovalue = isovalue;
        volume.color = color;

        uniform_buffer_volume_.write(
                command_pool, queue, OFFSET, SIZE, reinterpret_cast<const char*>(&volume) + OFFSET);
}

void VolumeBuffer::set_color_volume(
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const bool color_volume) const
{
        decltype(Volume().color_volume) v = color_volume ? 1 : 0;
        uniform_buffer_volume_.write(
                command_pool, queue, offsetof(Volume, color_volume), data_size(v), data_pointer(v));
}

void VolumeBuffer::set_lighting(
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const float ambient,
        const float metalness,
        const float roughness) const
{
        static_assert(offsetof(Volume, roughness) - offsetof(Volume, ambient) == 2 * sizeof(float));

        constexpr std::size_t OFFSET = offsetof(Volume, ambient);
        constexpr std::size_t SIZE = offsetof(Volume, roughness) + sizeof(Volume::roughness) - OFFSET;

        Volume volume;

        volume.ambient = ambient;
        volume.metalness = metalness;
        volume.roughness = roughness;

        uniform_buffer_volume_.write(
                command_pool, queue, OFFSET, SIZE, reinterpret_cast<const char*>(&volume) + OFFSET);
}

TransparencyBuffers::TransparencyBuffers(
        const vulkan::Device& device,
        const vulkan::CommandPool& command_pool,
        const vulkan::Queue& queue,
        const std::vector<std::uint32_t>& family_indices,
        const VkSampleCountFlagBits sample_count,
        const unsigned width,
        const unsigned height,
        const unsigned long long max_node_buffer_size)
        : node_count_(
                std::min(
                        max_node_buffer_size,
                        static_cast<unsigned long long>(device.properties().properties_10.limits.maxStorageBufferRange))
                / NODE_SIZE),
          heads_(device,
                 family_indices,
                 std::vector<VkFormat>({VK_FORMAT_R32_UINT}),
                 sample_count,
                 VK_IMAGE_TYPE_2D,
                 vulkan::make_extent(width, height),
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                 VK_IMAGE_LAYOUT_GENERAL,
                 command_pool,
                 queue),
          heads_size_(
                  device,
                  family_indices,
                  std::vector<VkFormat>({VK_FORMAT_R32_UINT}),
                  sample_count,
                  VK_IMAGE_TYPE_2D,
                  vulkan::make_extent(width, height),
                  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                  VK_IMAGE_LAYOUT_GENERAL,
                  command_pool,
                  queue),
          node_buffer_(
                  vulkan::BufferMemoryType::DEVICE_LOCAL,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                  node_count_ * NODE_SIZE),
          init_buffer_(
                  vulkan::BufferMemoryType::HOST_VISIBLE,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  sizeof(Counters)),
          read_buffer_(
                  vulkan::BufferMemoryType::HOST_VISIBLE,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  sizeof(Counters)),
          counters_(
                  vulkan::BufferMemoryType::DEVICE_LOCAL,
                  device,
                  family_indices,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
                          | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                  sizeof(Counters))
{
        Counters counters;
        counters.transparency_node_counter = 0;
        counters.transparency_overload_counter = 0;
        vulkan::BufferMapper mapper(init_buffer_, 0, init_buffer_.buffer().size());
        mapper.write(counters);
}

const vulkan::Buffer& TransparencyBuffers::counters() const
{
        return counters_.buffer();
}

const vulkan::ImageWithMemory& TransparencyBuffers::heads() const
{
        return heads_;
}

const vulkan::ImageWithMemory& TransparencyBuffers::heads_size() const
{
        return heads_size_;
}

const vulkan::Buffer& TransparencyBuffers::nodes() const
{
        return node_buffer_.buffer();
}

unsigned TransparencyBuffers::node_count() const
{
        return node_count_;
}

void TransparencyBuffers::commands_init(const VkCommandBuffer command_buffer) const
{
        commands_init_uint32_storage_image(command_buffer, heads_, HEADS_NULL_POINTER);
        commands_init_uint32_storage_image(command_buffer, heads_size_, 0);
        commands_init_buffer(command_buffer, init_buffer_, counters_);
}

void TransparencyBuffers::commands_read(const VkCommandBuffer command_buffer) const
{
        commands_read_buffer(command_buffer, counters_, read_buffer_);
}

void TransparencyBuffers::read(unsigned long long* const required_node_memory, unsigned* const overload_counter) const
{
        vulkan::BufferMapper mapper(read_buffer_);
        Counters counters;
        mapper.read(&counters);
        *required_node_memory = counters.transparency_node_counter * NODE_SIZE;
        *overload_counter = counters.transparency_overload_counter;
}
}
