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

#include "volume_object.h"

#include "shading.h"

#include "shaders/buffers.h"
#include "shaders/volume.h"

#include <src/image/format.h>
#include <src/vulkan/buffers.h>

#include <unordered_map>

namespace ns::gpu::renderer
{
namespace
{
constexpr double GRADIENT_H_IN_PIXELS = 0.5;

std::vector<VkFormat> vulkan_transfer_function_formats(image::ColorFormat color_format)
{
        switch (color_format)
        {
        case image::ColorFormat::R8G8B8A8_SRGB:
        case image::ColorFormat::R16G16B16A16:
        case image::ColorFormat::R32G32B32A32:
                return {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT};
        case image::ColorFormat::R16:
        case image::ColorFormat::R32:
        case image::ColorFormat::R8_SRGB:
        case image::ColorFormat::R8G8B8_SRGB:
        case image::ColorFormat::R16G16B16:
        case image::ColorFormat::R32G32B32:
                error("Unsupported transfer function format: " + image::format_to_string(color_format));
        }
        error_fatal("Unknown color format " + image::format_to_string(color_format));
}

std::vector<VkFormat> vulkan_image_formats(image::ColorFormat color_format)
{
        switch (color_format)
        {
        case image::ColorFormat::R16:
        case image::ColorFormat::R32:
                return {VK_FORMAT_R16_UNORM, VK_FORMAT_R32_SFLOAT};
        case image::ColorFormat::R8G8B8A8_SRGB:
                return {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT};
        case image::ColorFormat::R16G16B16A16:
        case image::ColorFormat::R32G32B32A32:
                return {VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R32G32B32A32_SFLOAT};
        case image::ColorFormat::R8_SRGB:
        case image::ColorFormat::R8G8B8_SRGB:
        case image::ColorFormat::R16G16B16:
        case image::ColorFormat::R32G32B32:
                error("Unsupported volume image format: " + image::format_to_string(color_format));
        }
        error_fatal("Unknown color format " + image::format_to_string(color_format));
}

bool is_scalar_volume(image::ColorFormat color_format)
{
        return 1 == image::format_component_count(color_format);
}

image::Image<1> transfer_function()
{
        constexpr int size = 256;
        constexpr Color color = Color(Srgb8(230, 255, 230));

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

        image::Image<1> image;

        image.pixels.resize(data_size(pixels));
        std::memcpy(image.pixels.data(), data_pointer(pixels), data_size(pixels));
        image.color_format = image::ColorFormat::R32G32B32A32;
        image.size[0] = size;

        return image;
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

vec3 world_volume_size(const mat4& texture_to_world_matrix)
{
        // Например, для x: texture_to_world_matrix * vec4(1, 0, 0, 1) -> vec3 -> length
        vec3 size;
        for (unsigned i = 0; i < 3; ++i)
        {
                vec3 v(texture_to_world_matrix(0, i), texture_to_world_matrix(1, i), texture_to_world_matrix(2, i));
                size[i] = v.norm();
        }
        return size;
}

// В текстурных координатах
vec3 gradient_h(const mat4& texture_to_world_matrix, const vulkan::ImageWithMemory& image)
{
        vec3 texture_pixel_size(1.0 / image.width(), 1.0 / image.height(), 1.0 / image.depth());

        vec3 world_pixel_size(texture_pixel_size * world_volume_size(texture_to_world_matrix));

        double min_world_pixel_size = world_pixel_size[0];
        for (unsigned i = 1; i < 3; ++i)
        {
                min_world_pixel_size = std::min(min_world_pixel_size, world_pixel_size[i]);
        }

        min_world_pixel_size *= GRADIENT_H_IN_PIXELS;

        vec3 h;
        for (unsigned i = 0; i < 3; ++i)
        {
                h[i] = (min_world_pixel_size / world_pixel_size[i]) * texture_pixel_size[i];
        }

        return h;
}

class Impl final : public VolumeObject
{
        const vulkan::Device& m_device;
        const vulkan::CommandPool& m_graphics_command_pool;
        const vulkan::Queue& m_graphics_queue;

        mat4 m_vp_matrix = mat4(1);
        std::optional<vec4> m_world_clip_plane_equation;

        mat3 m_object_normal_to_world_normal_matrix;
        mat4 m_texture_to_world_matrix;
        vec3 m_gradient_h;

        VolumeBuffer m_buffer;
        std::unique_ptr<vulkan::ImageWithMemory> m_image;
        image::ColorFormat m_image_color_format;
        std::unique_ptr<vulkan::ImageWithMemory> m_transfer_function;

        std::unordered_map<VkDescriptorSetLayout, vulkan::Descriptors> m_descriptor_sets;
        const std::vector<vulkan::DescriptorSetLayoutAndBindings> m_image_layouts;

        VkSampler m_image_sampler;
        VkSampler m_transfer_function_sampler;

        std::optional<int> m_version;

        const volume::Update::Flags UPDATE_PARAMETERS = []()
        {
                volume::Update::Flags flags;
                flags.set(volume::Update::Color);
                flags.set(volume::Update::Levels);
                flags.set(volume::Update::Isovalue);
                flags.set(volume::Update::Isosurface);
                flags.set(volume::Update::IsosurfaceAlpha);
                flags.set(volume::Update::VolumeAlphaCoefficient);
                return flags;
        }();

        const volume::Update::Flags UPDATE_LIGHTING = []()
        {
                volume::Update::Flags flags;
                flags.set(volume::Update::Ambient);
                flags.set(volume::Update::Diffuse);
                flags.set(volume::Update::Specular);
                flags.set(volume::Update::SpecularPower);
                return flags;
        }();

        void buffer_set_parameters(
                float window_min,
                float window_max,
                float volume_alpha_coeficient,
                float isosurface_alpha,
                bool isosurface,
                float isovalue,
                const Color& color) const
        {
                constexpr float eps = 1e-10f;
                window_min = std::min(std::max(0.0f, window_min), 1 - eps);
                window_max = std::max(std::min(1.0f, window_max), window_min + eps);
                float window_offset = window_min;
                float window_scale = 1 / (window_max - window_min);

                isovalue = std::clamp(isovalue, 0.0f, 1.0f);
                isosurface_alpha = std::clamp(isosurface_alpha, 0.0f, 1.0f);

                m_buffer.set_parameters(
                        m_graphics_command_pool, m_graphics_queue, window_offset, window_scale, volume_alpha_coeficient,
                        isosurface_alpha, isosurface, isovalue, color);
        }

        void buffer_set_lighting(float ambient, float diffuse, float specular, float specular_power) const
        {
                ShadingParameters p = shading_parameters(ambient, diffuse, specular, specular_power);

                m_buffer.set_lighting(
                        m_graphics_command_pool, m_graphics_queue, p.ambient, p.metalness, p.specular_power);
        }

        void buffer_set_coordinates() const
        {
                const mat4& mvp = m_vp_matrix * m_texture_to_world_matrix;
                const vec4& clip_plane =
                        m_world_clip_plane_equation
                                ? image_clip_plane(*m_world_clip_plane_equation, m_texture_to_world_matrix)
                                : vec4(0);

                m_buffer.set_coordinates(
                        mvp.inverse(), mvp.row(2), clip_plane, m_gradient_h, m_object_normal_to_world_normal_matrix);
        }

        void buffer_set_clip_plane() const
        {
                ASSERT(m_world_clip_plane_equation);
                m_buffer.set_clip_plane(image_clip_plane(*m_world_clip_plane_equation, m_texture_to_world_matrix));
        }

        void buffer_set_color_volume(bool color_volume) const
        {
                m_buffer.set_color_volume(m_graphics_command_pool, m_graphics_queue, color_volume);
        }

        void create_descriptor_sets()
        {
                VolumeImageMemory::CreateInfo info;
                info.buffer_coordinates = m_buffer.buffer_coordinates();
                info.buffer_coordinates_size = m_buffer.buffer_coordinates_size();
                info.buffer_volume = m_buffer.buffer_volume();
                info.buffer_volume_size = m_buffer.buffer_volume_size();
                info.image = m_image->image_view();
                info.transfer_function = m_transfer_function->image_view();

                m_descriptor_sets.clear();
                for (const vulkan::DescriptorSetLayoutAndBindings& layout : m_image_layouts)
                {
                        vulkan::Descriptors sets = VolumeImageMemory::create(
                                m_device, m_image_sampler, m_transfer_function_sampler, layout.descriptor_set_layout,
                                layout.descriptor_set_layout_bindings, info);

                        ASSERT(sets.descriptor_set_count() == 1);
                        m_descriptor_sets.emplace(sets.descriptor_set_layout(), std::move(sets));
                }
        }

        void set_transfer_function()
        {
                if (m_transfer_function)
                {
                        return;
                }

                image::Image<1> image = transfer_function();

                m_transfer_function.reset();

                m_transfer_function = std::make_unique<vulkan::ImageWithMemory>(
                        m_device, m_graphics_command_pool, m_graphics_queue,
                        std::unordered_set<uint32_t>{m_graphics_queue.family_index()},
                        vulkan_transfer_function_formats(image.color_format), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_1D,
                        vulkan::make_extent(image.size[0]), VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

                m_transfer_function->write_pixels(
                        m_graphics_command_pool, m_graphics_queue, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image.color_format, image.pixels);
        }

        void set_image(const image::Image<3>& image, bool* size_changed)
        {
                VkImageLayout image_layout;

                if (!m_image || m_image_color_format != image.color_format
                    || m_image->width() != static_cast<unsigned>(image.size[0])
                    || m_image->height() != static_cast<unsigned>(image.size[1])
                    || m_image->depth() != static_cast<unsigned>(image.size[2]))
                {
                        *size_changed = true;

                        buffer_set_color_volume(!is_scalar_volume(image.color_format));

                        m_image_color_format = image.color_format;

                        m_image.reset();
                        m_image = std::make_unique<vulkan::ImageWithMemory>(
                                m_device, m_graphics_command_pool, m_graphics_queue,
                                std::unordered_set<uint32_t>({m_graphics_queue.family_index()}),
                                vulkan_image_formats(image.color_format), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_3D,
                                vulkan::make_extent(image.size[0], image.size[1], image.size[2]),
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

                        image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
                }
                else
                {
                        *size_changed = false;

                        image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }

                m_image->write_pixels(
                        m_graphics_command_pool, m_graphics_queue, image_layout,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image.color_format, image.pixels);
        }

        const VkDescriptorSet& descriptor_set(VkDescriptorSetLayout descriptor_set_layout) const override
        {
                auto iter = m_descriptor_sets.find(descriptor_set_layout);
                if (iter == m_descriptor_sets.cend())
                {
                        error("Failed to find volume descriptor set for descriptor set layout");
                }
                ASSERT(iter->second.descriptor_set_count() == 1);
                return iter->second.descriptor_set(0);
        }

        void set_matrix_and_clip_plane(const mat4& vp_matrix, const std::optional<vec4>& world_clip_plane_equation)
                override
        {
                m_vp_matrix = vp_matrix;
                m_world_clip_plane_equation = world_clip_plane_equation;
                buffer_set_coordinates();
        }

        void set_clip_plane(const vec4& world_clip_plane_equation) override
        {
                m_world_clip_plane_equation = world_clip_plane_equation;
                buffer_set_clip_plane();
        }

        UpdateChanges update(const volume::Reading<3>& volume_object) override
        {
                UpdateChanges update_changes;

                volume::Update::Flags updates;
                volume_object.updates(&m_version, &updates);
                if (updates.none())
                {
                        return update_changes;
                }

                static_assert(volume::Update::Flags().size() == 12);

                bool update_image = updates[volume::Update::Image];
                bool update_matrices = updates[volume::Update::Matrices];
                bool update_parameters = (updates & UPDATE_PARAMETERS).any();
                bool update_ligthing = (updates & UPDATE_LIGHTING).any();

                if (update_image)
                {
                        set_transfer_function();

                        bool size_changed;
                        set_image(volume_object.volume().image, &size_changed);
                        update_matrices = update_matrices || size_changed;

                        create_descriptor_sets();

                        update_changes.command_buffers = true;
                }

                if (update_parameters)
                {
                        buffer_set_parameters(
                                volume_object.level_min(), volume_object.level_max(),
                                volume_object.volume_alpha_coefficient(), volume_object.isosurface_alpha(),
                                volume_object.isosurface(), volume_object.isovalue(), volume_object.color());
                }

                if (update_ligthing)
                {
                        buffer_set_lighting(
                                volume_object.ambient(), volume_object.diffuse(), volume_object.specular(),
                                volume_object.specular_power());
                }

                if (update_matrices)
                {
                        m_object_normal_to_world_normal_matrix =
                                volume_object.matrix().top_left<3, 3>().inverse().transpose();
                        m_texture_to_world_matrix = volume_object.matrix() * volume_object.volume().matrix;
                        m_gradient_h = gradient_h(m_texture_to_world_matrix, *m_image);

                        buffer_set_coordinates();
                }

                return update_changes;
        }

public:
        Impl(const vulkan::Device& device,
             const vulkan::CommandPool& graphics_command_pool,
             const vulkan::Queue& graphics_queue,
             const vulkan::CommandPool& /*transfer_command_pool*/,
             const vulkan::Queue& /*transfer_queue*/,
             std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts,
             VkSampler image_sampler,
             VkSampler transfer_function_sampler)
                : m_device(device),
                  m_graphics_command_pool(graphics_command_pool),
                  m_graphics_queue(graphics_queue),
                  m_buffer(m_device, {m_graphics_queue.family_index()}),
                  m_image_layouts(std::move(image_layouts)),
                  m_image_sampler(image_sampler),
                  m_transfer_function_sampler(transfer_function_sampler)
        {
        }
};
}

std::unique_ptr<VolumeObject> create_volume_object(
        const vulkan::Device& device,
        const vulkan::CommandPool& graphics_command_pool,
        const vulkan::Queue& graphics_queue,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts,
        VkSampler image_sampler,
        VkSampler transfer_function_sampler)
{
        return std::make_unique<Impl>(
                device, graphics_command_pool, graphics_queue, transfer_command_pool, transfer_queue,
                std::move(image_layouts), image_sampler, transfer_function_sampler);
}
}
