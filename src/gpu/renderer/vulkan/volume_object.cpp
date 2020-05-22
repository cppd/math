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

#include <src/image/format.h>
#include <src/vulkan/buffers.h>

#include <unordered_map>

namespace gpu::renderer
{
namespace
{
// clang-format off
constexpr std::initializer_list<VkFormat> SCALAR_FORMATS =
{
        VK_FORMAT_R16_UNORM,
        VK_FORMAT_R32_SFLOAT
};
constexpr std::initializer_list<VkFormat> COLOR_FORMATS =
{
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_FORMAT_R16G16B16A16_UNORM,
        VK_FORMAT_R32G32B32A32_SFLOAT
};
// clang-format on

// цвет RGBA
void transfer_function(image::ColorFormat* color_format, std::vector<std::byte>* bytes)
{
        const int size = 256;
        const Color color(Srgb8(230, 255, 230));

        std::vector<float> pixels;
        const float max = size - 1;
        for (int i = 0; i < size; ++i)
        {
                float alpha = i / max;
                pixels.push_back(color.red());
                pixels.push_back(color.green());
                pixels.push_back(color.blue());
                pixels.push_back(alpha);
        }

        bytes->resize(data_size(pixels));
        std::memcpy(bytes->data(), data_pointer(pixels), data_size(pixels));
        *color_format = image::ColorFormat::R32G32B32A32;
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

enum class Memory
{
        Yes,
        No
};

void find_image_formats_and_volume_type(
        image::ColorFormat color_format,
        std::vector<VkFormat>* formats,
        bool* color_volume)
{
        switch (color_format)
        {
        case image::ColorFormat::R16:
        case image::ColorFormat::R32:
                *formats = SCALAR_FORMATS;
                *color_volume = false;
                return;
        case image::ColorFormat::R8G8B8A8_SRGB:
        case image::ColorFormat::R16G16B16A16:
        case image::ColorFormat::R32G32B32A32:
                *formats = COLOR_FORMATS;
                *color_volume = true;
                return;
        case image::ColorFormat::R8_SRGB:
        case image::ColorFormat::R8G8B8_SRGB:
        case image::ColorFormat::R16G16B16:
        case image::ColorFormat::R32G32B32:
                error("Unsupported volume image format: " + image::format_to_string(color_format));
        }
        error_fatal("Unknown color format " + image::format_to_string(color_format));
}
}

class VolumeObject::Volume
{
        const vulkan::Device& m_device;
        const vulkan::CommandPool& m_graphics_command_pool;
        const vulkan::Queue& m_graphics_queue;

        mat4 m_vp_matrix = mat4(1);
        std::optional<vec4> m_world_clip_plane_equation;

        mat4 m_model_matrix;

        VolumeBuffer m_buffer;
        std::unique_ptr<vulkan::ImageWithMemory> m_image;
        image::ColorFormat m_image_color_format;
        std::unique_ptr<vulkan::ImageWithMemory> m_transfer_function;
        std::unordered_map<VkDescriptorSetLayout, VolumeImageMemory> m_memory;
        std::function<VolumeImageMemory(const VolumeInfo&)> m_create_descriptor_sets;

        void buffer_set_parameters(float window_min, float window_max, float transparency) const
        {
                constexpr float eps = 1e-10f;
                window_min = std::min(std::max(0.0f, window_min), 1 - eps);
                window_max = std::max(std::min(1.0f, window_max), window_min + eps);
                float window_offset = window_min;
                float window_scale = 1 / (window_max - window_min);

                m_buffer.set_parameters(
                        m_graphics_command_pool, m_graphics_queue, window_offset, window_scale, transparency);
        }

        void buffer_set_matrix_and_clip_plane() const
        {
                const mat4& mvp = m_vp_matrix * m_model_matrix;
                const vec4& clip_plane = m_world_clip_plane_equation
                                                 ? image_clip_plane(*m_world_clip_plane_equation, m_model_matrix)
                                                 : vec4(0);
                m_buffer.set_matrix_and_clip_plane(mvp.inverse(), clip_plane);
        }

        void buffer_set_clip_plane() const
        {
                ASSERT(m_world_clip_plane_equation);
                m_buffer.set_clip_plane(image_clip_plane(*m_world_clip_plane_equation, m_model_matrix));
        }

        void buffer_set_color_volume(bool color_volume) const
        {
                m_buffer.set_color_volume(m_graphics_command_pool, m_graphics_queue, color_volume);
        }

        void create_memory()
        {
                VolumeInfo info;
                info.buffer_coordinates = m_buffer.buffer_coordinates();
                info.buffer_coordinates_size = m_buffer.buffer_coordinates_size();
                info.buffer_volume = m_buffer.buffer_volume();
                info.buffer_volume_size = m_buffer.buffer_volume_size();
                info.image = m_image->image_view();
                info.transfer_function = m_transfer_function->image_view();

                VolumeImageMemory memory = m_create_descriptor_sets(info);

                m_memory.erase(memory.descriptor_set_layout());
                m_memory.emplace(memory.descriptor_set_layout(), std::move(memory));
        }

        void set_transfer_function(Memory with_memory_creation)
        {
                image::ColorFormat color_format;
                std::vector<std::byte> color_bytes;
                transfer_function(&color_format, &color_bytes);
                unsigned pixel_count = color_bytes.size() / image::format_pixel_size_in_bytes(color_format);

                m_transfer_function.reset();

                m_transfer_function = std::make_unique<vulkan::ImageWithMemory>(
                        m_device, m_graphics_command_pool, m_graphics_queue,
                        std::unordered_set<uint32_t>{m_graphics_queue.family_index()}, COLOR_FORMATS,
                        VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_1D, vulkan::make_extent(pixel_count),
                        VK_IMAGE_LAYOUT_UNDEFINED, false /*storage*/);

                m_transfer_function->write_pixels(
                        m_graphics_command_pool, m_graphics_queue, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, color_format, color_bytes);

                if (with_memory_creation == Memory::Yes)
                {
                        create_memory();
                }
        }

        void set_image(const image::Image<3>& image, Memory with_memory_creation)
        {
                VkImageLayout image_layout;
                if (!m_image || m_image_color_format != image.color_format
                    || m_image->width() != static_cast<unsigned>(image.size[0])
                    || m_image->height() != static_cast<unsigned>(image.size[1])
                    || m_image->depth() != static_cast<unsigned>(image.size[2]))
                {
                        std::vector<VkFormat> formats;
                        bool color_volume;

                        find_image_formats_and_volume_type(image.color_format, &formats, &color_volume);

                        buffer_set_color_volume(color_volume);

                        m_image_color_format = image.color_format;

                        m_image.reset();
                        m_image = std::make_unique<vulkan::ImageWithMemory>(
                                m_device, m_graphics_command_pool, m_graphics_queue,
                                std::unordered_set<uint32_t>({m_graphics_queue.family_index()}), formats,
                                VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_3D,
                                vulkan::make_extent(image.size[0], image.size[1], image.size[2]),
                                VK_IMAGE_LAYOUT_UNDEFINED, false /*storage*/);

                        ASSERT((m_image->usage() & VK_IMAGE_USAGE_SAMPLED_BIT) == VK_IMAGE_USAGE_SAMPLED_BIT);
                        ASSERT((m_image->usage() & VK_IMAGE_USAGE_STORAGE_BIT) == 0);

                        image_layout = VK_IMAGE_LAYOUT_UNDEFINED;

                        if (with_memory_creation == Memory::Yes)
                        {
                                create_memory();
                        }
                }
                else
                {
                        image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }

                m_image->write_pixels(
                        m_graphics_command_pool, m_graphics_queue, image_layout,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image.color_format, image.pixels);
        }

public:
        Volume(const vulkan::Device& device,
               const vulkan::CommandPool& graphics_command_pool,
               const vulkan::Queue& graphics_queue,
               const vulkan::CommandPool& /*transfer_command_pool*/,
               const vulkan::Queue& /*transfer_queue*/,
               const std::function<VolumeImageMemory(const VolumeInfo&)>& create_descriptor_sets)
                : m_device(device),
                  m_graphics_command_pool(graphics_command_pool),
                  m_graphics_queue(graphics_queue),
                  m_buffer(m_device, {m_graphics_queue.family_index()}),
                  m_create_descriptor_sets(create_descriptor_sets)
        {
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

        void set_matrix_and_clip_plane(const mat4& vp_matrix, const std::optional<vec4>& world_clip_plane_equation)
        {
                m_vp_matrix = vp_matrix;
                m_world_clip_plane_equation = world_clip_plane_equation;
                buffer_set_matrix_and_clip_plane();
        }

        void set_clip_plane(const vec4& world_clip_plane_equation)
        {
                m_world_clip_plane_equation = world_clip_plane_equation;
                buffer_set_clip_plane();
        }

        void update(
                const std::unordered_set<volume::Update>& updates,
                const volume::VolumeObject<3>& volume_object,
                bool* update_command_buffers)
        {
                *update_command_buffers = false;

                if (updates.empty())
                {
                        return;
                }

                if (updates.contains(volume::Update::All))
                {
                        m_model_matrix = volume_object.matrix() * volume_object.volume().matrix;
                        buffer_set_matrix_and_clip_plane();

                        buffer_set_parameters(
                                volume_object.level_min(), volume_object.level_max(), volume_object.transparency());

                        set_transfer_function(Memory::No);
                        set_image(volume_object.volume().image, Memory::No);

                        create_memory();

                        *update_command_buffers = true;
                }
                else
                {
                        if (updates.contains(volume::Update::Image))
                        {
                                set_image(volume_object.volume().image, Memory::Yes);
                                *update_command_buffers = true;
                        }

                        if (updates.contains(volume::Update::Parameters))
                        {
                                buffer_set_parameters(
                                        volume_object.level_min(), volume_object.level_max(),
                                        volume_object.transparency());
                        }

                        if (updates.contains(volume::Update::Matrices))
                        {
                                m_model_matrix = volume_object.matrix() * volume_object.volume().matrix;
                                buffer_set_matrix_and_clip_plane();
                        }
                }

                ASSERT([&updates]() {
                        std::unordered_set<volume::Update> s = updates;
                        s.erase(volume::Update::All);
                        s.erase(volume::Update::Image);
                        s.erase(volume::Update::Parameters);
                        s.erase(volume::Update::Matrices);
                        return s.empty();
                }());
        }
};

VolumeObject::VolumeObject(
        const vulkan::Device& device,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        const std::function<VolumeImageMemory(const VolumeInfo&)>& create_descriptor_sets)
{
        m_volume = std::make_unique<Volume>(
                device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                create_descriptor_sets);
}

VolumeObject::~VolumeObject() = default;

const VkDescriptorSet& VolumeObject::descriptor_set(VkDescriptorSetLayout descriptor_set_layout) const
{
        return m_volume->descriptor_set(descriptor_set_layout);
}

void VolumeObject::set_matrix_and_clip_plane(
        const mat4& vp_matrix,
        const std::optional<vec4>& world_clip_plane_equation)
{
        m_volume->set_matrix_and_clip_plane(vp_matrix, world_clip_plane_equation);
}

void VolumeObject::set_clip_plane(const vec4& world_clip_plane_equation)
{
        m_volume->set_clip_plane(world_clip_plane_equation);
}

void VolumeObject::update(
        const std::unordered_set<volume::Update>& updates,
        const volume::VolumeObject<3>& volume_object,
        bool* update_command_buffers)
{
        m_volume->update(updates, volume_object, update_command_buffers);
}
}
