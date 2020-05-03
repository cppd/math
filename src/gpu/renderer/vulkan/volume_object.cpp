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

#include "volume_object.h"

#include <src/color/conversion.h>
#include <src/vulkan/buffers.h>

#include <unordered_map>

namespace gpu::renderer
{
namespace
{
// clang-format off
constexpr std::initializer_list<VkFormat> IMAGE_FORMATS =
{
        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R32_SFLOAT
};
constexpr std::initializer_list<VkFormat> TRANSFER_FUNCTION_FORMATS =
{
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R32G32B32A32_SFLOAT
};
// clang-format on

// цвет RGBA, умноженный на α, в пространстве sRGB
std::vector<std::uint8_t> transfer_function()
{
        std::vector<std::uint8_t> pixels;
        const int size = 1024;
        const int max = size - 1;
        const float max_r = 1.0f / max;
        for (int i = 0; i <= max; ++i)
        {
                float v = i * max_r;
                uint8_t c = color_conversion::rgb_float_to_srgb_uint8(v);
                uint8_t a = std::round(v * 255);
                pixels.push_back(c);
                pixels.push_back(c);
                pixels.push_back(c);
                pixels.push_back(a);
        }
        return pixels;
}

vec4 image_clip_plane(const vec4& world_clip_plane, const mat4& model)
{
        vec4 p = world_clip_plane * model;

        // из уравнения n * x + d с нормалью внутрь
        // в уравнение n * x - d с нормалью наружу
        p[3] = -p[3];
        vec3 n = vec3(p[0], p[1], p[2]);
        return p / -n.norm();
}
}

class VolumeObject::Volume
{
        const vulkan::CommandPool& m_graphics_command_pool;
        const vulkan::Queue& m_graphics_queue;

        mat4 m_model_volume_matrix;

        VolumeBuffer m_buffer;
        vulkan::ImageWithMemory m_image;
        std::unique_ptr<vulkan::ImageWithMemory> m_transfer_function;

        std::unordered_map<VkDescriptorSetLayout, VolumeImageMemory> m_memory;

        std::function<VolumeImageMemory(const VolumeInfo&)> m_create_descriptor_sets;

        void set_window(float window_min, float window_max)
        {
                constexpr float eps = 1e-10f;
                window_min = std::min(std::max(0.0f, window_min), 1 - eps);
                window_max = std::max(std::min(1.0f, window_max), window_min + eps);
                float window_offset = window_min;
                float window_scale = 1 / (window_max - window_min);
                m_buffer.set_window(m_graphics_command_pool, m_graphics_queue, window_offset, window_scale);
        }

        void create_memory()
        {
                VolumeInfo info;
                info.buffer_coordinates = m_buffer.buffer_coordinates();
                info.buffer_coordinates_size = m_buffer.buffer_coordinates_size();
                info.buffer_volume = m_buffer.buffer_volume();
                info.buffer_volume_size = m_buffer.buffer_volume_size();
                info.image = m_image.image_view();
                info.transfer_function = m_transfer_function->image_view();

                VolumeImageMemory memory = m_create_descriptor_sets(info);

                m_memory.erase(memory.descriptor_set_layout());
                m_memory.emplace(memory.descriptor_set_layout(), std::move(memory));
        }

public:
        Volume(const vulkan::Device& device,
               const vulkan::CommandPool& graphics_command_pool,
               const vulkan::Queue& graphics_queue,
               const vulkan::CommandPool& /*transfer_command_pool*/,
               const vulkan::Queue& /*transfer_queue*/,
               const volume::VolumeObject<3>& volume_object,
               const std::function<VolumeImageMemory(const VolumeInfo&)>& create_descriptor_sets)
                : m_graphics_command_pool(graphics_command_pool),
                  m_graphics_queue(graphics_queue),
                  m_model_volume_matrix(volume_object.matrix() * volume_object.volume().matrix),
                  m_buffer(device, {graphics_queue.family_index()}),
                  m_image(device,
                          graphics_command_pool,
                          graphics_queue,
                          {graphics_queue.family_index()},
                          IMAGE_FORMATS,
                          VK_SAMPLE_COUNT_1_BIT,
                          VK_IMAGE_TYPE_3D,
                          vulkan::make_extent(
                                  volume_object.volume().image.size[0],
                                  volume_object.volume().image.size[1],
                                  volume_object.volume().image.size[2]),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          false /*storage*/),
                  m_create_descriptor_sets(create_descriptor_sets)
        {
                m_image.write_linear_grayscale_pixels(
                        graphics_command_pool, graphics_queue, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, volume_object.volume().image.pixels);

                ASSERT((m_image.usage() & VK_IMAGE_USAGE_SAMPLED_BIT) == VK_IMAGE_USAGE_SAMPLED_BIT);
                ASSERT((m_image.usage() & VK_IMAGE_USAGE_STORAGE_BIT) == 0);

                set_window(volume_object.level_min(), volume_object.level_max());

                std::vector<uint8_t> transfer_function_pixels = transfer_function();
                m_transfer_function = std::make_unique<vulkan::ImageWithMemory>(
                        device, graphics_command_pool, graphics_queue,
                        std::unordered_set<uint32_t>{graphics_queue.family_index()}, TRANSFER_FUNCTION_FORMATS,
                        VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_1D,
                        vulkan::make_extent(transfer_function_pixels.size() / 4), VK_IMAGE_LAYOUT_UNDEFINED,
                        false /*storage*/);
                m_transfer_function->write_srgb_color_pixels(
                        graphics_command_pool, graphics_queue, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, transfer_function_pixels);

                create_memory();
        }

        const VkDescriptorSet& descriptor_set(VkDescriptorSetLayout descriptor_set_layout) const
        {
                auto iter = m_memory.find(descriptor_set_layout);
                if (iter == m_memory.cend())
                {
                        error("Failed to find volume descriptor set for descriptor set layout");
                }

                return iter->second.descriptor_set();
        }

        void set_coordinates(const mat4& vp_matrix, const std::optional<vec4>& world_clip_plane_equation) const
        {
                const mat4& model = m_model_volume_matrix;
                const mat4& mvp = vp_matrix * model;
                const vec4& clip_plane =
                        world_clip_plane_equation ? image_clip_plane(*world_clip_plane_equation, model) : vec4(0);
                m_buffer.set_matrix_and_clip_plane(mvp.inverse(), clip_plane);
        }

        void set_clip_plane(const vec4& world_clip_plane_equation) const
        {
                m_buffer.set_clip_plane(image_clip_plane(world_clip_plane_equation, m_model_volume_matrix));
        }
};

VolumeObject::VolumeObject(
        const vulkan::Device& device,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        const volume::VolumeObject<3>& volume_object,
        const std::function<VolumeImageMemory(const VolumeInfo&)>& create_descriptor_sets)
{
        m_volume = std::make_unique<Volume>(
                device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue, volume_object,
                create_descriptor_sets);
}

VolumeObject::~VolumeObject() = default;

const VkDescriptorSet& VolumeObject::descriptor_set(VkDescriptorSetLayout descriptor_set_layout) const
{
        return m_volume->descriptor_set(descriptor_set_layout);
}

void VolumeObject::set_coordinates(const mat4& vp_matrix, const std::optional<vec4>& world_clip_plane_equation) const
{
        m_volume->set_coordinates(vp_matrix, world_clip_plane_equation);
}

void VolumeObject::set_clip_plane(const vec4& world_clip_plane_equation) const
{
        m_volume->set_clip_plane(world_clip_plane_equation);
}
}
