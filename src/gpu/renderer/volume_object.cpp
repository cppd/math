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

#include "shading_parameters.h"

#include "shaders/buffers.h"
#include "shaders/volume.h"

#include <src/color/conversion.h>
#include <src/com/alg.h>
#include <src/com/merge.h>
#include <src/image/conversion.h>
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
        case image::ColorFormat::R16G16B16A16_SRGB:
        case image::ColorFormat::R32G32B32A32:
                return {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT};
        case image::ColorFormat::R16:
        case image::ColorFormat::R32:
        case image::ColorFormat::R8_SRGB:
        case image::ColorFormat::R8G8B8_SRGB:
        case image::ColorFormat::R16G16B16:
        case image::ColorFormat::R16G16B16_SRGB:
        case image::ColorFormat::R32G32B32:
        case image::ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
        case image::ColorFormat::R16G16B16A16_PREMULTIPLIED:
        case image::ColorFormat::R32G32B32A32_PREMULTIPLIED:
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
        case image::ColorFormat::R8G8B8_SRGB:
        case image::ColorFormat::R8G8B8A8_SRGB:
        case image::ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
                return {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R32G32B32A32_SFLOAT};
        case image::ColorFormat::R16G16B16:
        case image::ColorFormat::R16G16B16_SRGB:
        case image::ColorFormat::R16G16B16A16:
        case image::ColorFormat::R16G16B16A16_SRGB:
        case image::ColorFormat::R16G16B16A16_PREMULTIPLIED:
        case image::ColorFormat::R32G32B32:
        case image::ColorFormat::R32G32B32A32:
        case image::ColorFormat::R32G32B32A32_PREMULTIPLIED:
                return {VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_R32G32B32A32_SFLOAT};
        case image::ColorFormat::R8_SRGB:
                error("Unsupported volume image format: " + image::format_to_string(color_format));
        }
        error_fatal("Unknown color format " + image::format_to_string(color_format));
}

void write_to_buffer_image(
        const image::Image<3>& image,
        const std::function<void(image::ColorFormat color_format, const std::vector<std::byte>& pixels)>& write)
{
        switch (image.color_format)
        {
        case image::ColorFormat::R16:
        case image::ColorFormat::R32:
        case image::ColorFormat::R8G8B8A8_SRGB:
        case image::ColorFormat::R16G16B16A16:
        case image::ColorFormat::R16G16B16A16_SRGB:
        case image::ColorFormat::R32G32B32A32:
                write(image.color_format, image.pixels);
                return;
        case image::ColorFormat::R8G8B8_SRGB:
        case image::ColorFormat::R16G16B16:
        case image::ColorFormat::R16G16B16_SRGB:
        case image::ColorFormat::R32G32B32:
        case image::ColorFormat::R8G8B8A8_SRGB_PREMULTIPLIED:
        case image::ColorFormat::R16G16B16A16_PREMULTIPLIED:
        case image::ColorFormat::R32G32B32A32_PREMULTIPLIED:
        {
                constexpr image::ColorFormat COLOR_FORMAT = image::ColorFormat::R32G32B32A32;
                std::vector<std::byte> pixels;
                image::format_conversion(image.color_format, image.pixels, COLOR_FORMAT, &pixels);
                write(COLOR_FORMAT, pixels);
                return;
        }
        case image::ColorFormat::R8_SRGB:
                error("Unsupported volume image format: " + image::format_to_string(image.color_format));
        }
        error_fatal("Unknown color format " + image::format_to_string(image.color_format));
}

bool is_scalar_volume(image::ColorFormat color_format)
{
        return 1 == image::format_component_count(color_format);
}

image::Image<1> transfer_function()
{
        constexpr int SIZE = 256;
        constexpr RGB8 COLOR = RGB8(230, 255, 230);

        constexpr float RED = color::srgb_uint8_to_linear_float(COLOR.red);
        constexpr float GREEN = color::srgb_uint8_to_linear_float(COLOR.green);
        constexpr float BLUE = color::srgb_uint8_to_linear_float(COLOR.blue);

        std::vector<float> pixels;
        pixels.reserve(4 * SIZE);
        constexpr float MAX = SIZE - 1;
        for (int i = 0; i < SIZE; ++i)
        {
                float alpha = i / MAX;
                pixels.push_back(RED);
                pixels.push_back(GREEN);
                pixels.push_back(BLUE);
                pixels.push_back(alpha);
        }

        image::Image<1> image;

        image.pixels.resize(data_size(pixels));
        std::memcpy(image.pixels.data(), data_pointer(pixels), data_size(pixels));
        image.color_format = image::ColorFormat::R32G32B32A32;
        image.size[0] = SIZE;

        return image;
}

Vector4d image_clip_plane(const Vector4d& world_clip_plane, const Matrix4d& model)
{
        Vector4d p = world_clip_plane * model;

        // from n * x + d with normal directed inward
        // to n * x - d with normal directed outward
        p[3] = -p[3];
        Vector3d n = Vector3d(p[0], p[1], p[2]);
        return p / -n.norm();
}

Vector3d world_volume_size(const Matrix4d& texture_to_world_matrix)
{
        // Example for x: texture_to_world_matrix * (1, 0, 0, 1) -> (x, y, z) -> length
        Vector3d size;
        for (unsigned i = 0; i < 3; ++i)
        {
                Vector3d v(texture_to_world_matrix(0, i), texture_to_world_matrix(1, i), texture_to_world_matrix(2, i));
                size[i] = v.norm();
        }
        return size;
}

// in texture coordinates
Vector3d gradient_h(const Matrix4d& texture_to_world_matrix, const vulkan::ImageWithMemory& image)
{
        Vector3d texture_pixel_size(1.0 / image.width(), 1.0 / image.height(), 1.0 / image.depth());

        Vector3d world_pixel_size(texture_pixel_size * world_volume_size(texture_to_world_matrix));

        double min_world_pixel_size = world_pixel_size[0];
        for (unsigned i = 1; i < 3; ++i)
        {
                min_world_pixel_size = std::min(min_world_pixel_size, world_pixel_size[i]);
        }

        min_world_pixel_size *= GRADIENT_H_IN_PIXELS;

        Vector3d h;
        for (unsigned i = 0; i < 3; ++i)
        {
                h[i] = (min_world_pixel_size / world_pixel_size[i]) * texture_pixel_size[i];
        }

        return h;
}

class Impl final : public VolumeObject
{
        const vulkan::Device& device_;
        const std::vector<uint32_t> family_indices_;
        const vulkan::CommandPool& transfer_command_pool_;
        const vulkan::Queue& transfer_queue_;

        Matrix4d vp_matrix_ = Matrix4d(1);
        std::optional<Vector4d> world_clip_plane_equation_;

        Matrix3d object_normal_to_world_normal_matrix_;
        Matrix4d texture_to_world_matrix_;
        Vector3d gradient_h_;

        VolumeBuffer buffer_;
        std::unique_ptr<vulkan::ImageWithMemory> image_;
        std::vector<VkFormat> image_formats_;
        std::unique_ptr<vulkan::ImageWithMemory> transfer_function_;

        std::unordered_map<VkDescriptorSetLayout, vulkan::Descriptors> descriptor_sets_;
        const std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts_;

        VkSampler image_sampler_;
        VkSampler transfer_function_sampler_;

        std::optional<int> version_;

        void buffer_set_parameters(
                float window_min,
                float window_max,
                float volume_alpha_coefficient,
                float isosurface_alpha,
                bool isosurface,
                float isovalue,
                const color::Color& color) const
        {
                constexpr float EPS = 1e-10f;
                window_min = std::min(std::max(0.0f, window_min), 1 - EPS);
                window_max = std::max(std::min(1.0f, window_max), window_min + EPS);
                float window_offset = window_min;
                float window_scale = 1 / (window_max - window_min);

                isovalue = std::clamp(isovalue, 0.0f, 1.0f);
                isosurface_alpha = std::clamp(isosurface_alpha, 0.0f, 1.0f);

                buffer_.set_parameters(
                        transfer_command_pool_, transfer_queue_, window_offset, window_scale, volume_alpha_coefficient,
                        isosurface_alpha, isosurface, isovalue, color.rgb32().clamp(0, 1));
        }

        void buffer_set_lighting(float ambient, float metalness, float roughness) const
        {
                std::tie(ambient, metalness, roughness) = clean_shading_parameters(ambient, metalness, roughness);

                buffer_.set_lighting(transfer_command_pool_, transfer_queue_, ambient, metalness, roughness);
        }

        void buffer_set_coordinates() const
        {
                const Matrix4d& mvp = vp_matrix_ * texture_to_world_matrix_;
                const Vector4d& clip_plane =
                        world_clip_plane_equation_
                                ? image_clip_plane(*world_clip_plane_equation_, texture_to_world_matrix_)
                                : Vector4d(0);

                buffer_.set_coordinates(
                        mvp.inverse(), mvp.row(2), clip_plane, gradient_h_, object_normal_to_world_normal_matrix_);
        }

        void buffer_set_clip_plane() const
        {
                ASSERT(world_clip_plane_equation_);
                buffer_.set_clip_plane(image_clip_plane(*world_clip_plane_equation_, texture_to_world_matrix_));
        }

        void buffer_set_color_volume(bool color_volume) const
        {
                buffer_.set_color_volume(transfer_command_pool_, transfer_queue_, color_volume);
        }

        void create_descriptor_sets()
        {
                VolumeImageMemory::CreateInfo info;
                info.buffer_coordinates = buffer_.buffer_coordinates();
                info.buffer_coordinates_size = buffer_.buffer_coordinates_size();
                info.buffer_volume = buffer_.buffer_volume();
                info.buffer_volume_size = buffer_.buffer_volume_size();
                info.image = image_->image_view();
                info.transfer_function = transfer_function_->image_view();

                descriptor_sets_.clear();
                for (const vulkan::DescriptorSetLayoutAndBindings& layout : image_layouts_)
                {
                        vulkan::Descriptors sets = VolumeImageMemory::create(
                                device_, image_sampler_, transfer_function_sampler_, layout.descriptor_set_layout,
                                layout.descriptor_set_layout_bindings, info);

                        ASSERT(sets.descriptor_set_count() == 1);
                        descriptor_sets_.emplace(sets.descriptor_set_layout(), std::move(sets));
                }
        }

        void set_transfer_function()
        {
                if (transfer_function_)
                {
                        return;
                }

                image::Image<1> image = transfer_function();

                transfer_function_.reset();

                transfer_function_ = std::make_unique<vulkan::ImageWithMemory>(
                        device_, family_indices_, vulkan_transfer_function_formats(image.color_format),
                        VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_1D, vulkan::make_extent(image.size[0]),
                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                        transfer_command_pool_, transfer_queue_);

                transfer_function_->write_pixels(
                        transfer_command_pool_, transfer_queue_, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image.color_format, image.pixels);
        }

        void set_image(const image::Image<3>& image, bool* size_changed)
        {
                VkImageLayout image_layout;

                if (!image_ || image_formats_ != vulkan_image_formats(image.color_format)
                    || image_->width() != static_cast<unsigned>(image.size[0])
                    || image_->height() != static_cast<unsigned>(image.size[1])
                    || image_->depth() != static_cast<unsigned>(image.size[2]))
                {
                        *size_changed = true;

                        buffer_set_color_volume(!is_scalar_volume(image.color_format));

                        image_formats_ = vulkan_image_formats(image.color_format);

                        image_.reset();
                        image_ = std::make_unique<vulkan::ImageWithMemory>(
                                device_, family_indices_, image_formats_, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_3D,
                                vulkan::make_extent(image.size[0], image.size[1], image.size[2]),
                                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                                transfer_command_pool_, transfer_queue_);

                        image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
                }
                else
                {
                        *size_changed = false;

                        image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }

                write_to_buffer_image(
                        image,
                        [this, &image_layout](image::ColorFormat color_format, const std::vector<std::byte>& pixels)
                        {
                                image_->write_pixels(
                                        transfer_command_pool_, transfer_queue_, image_layout,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, color_format, pixels);
                        });
        }

        const VkDescriptorSet& descriptor_set(VkDescriptorSetLayout descriptor_set_layout) const override
        {
                auto iter = descriptor_sets_.find(descriptor_set_layout);
                if (iter == descriptor_sets_.cend())
                {
                        error("Failed to find volume descriptor set for descriptor set layout");
                }
                ASSERT(iter->second.descriptor_set_count() == 1);
                return iter->second.descriptor_set(0);
        }

        void set_matrix_and_clip_plane(
                const Matrix4d& vp_matrix,
                const std::optional<Vector4d>& world_clip_plane_equation) override
        {
                vp_matrix_ = vp_matrix;
                world_clip_plane_equation_ = world_clip_plane_equation;
                buffer_set_coordinates();
        }

        void set_clip_plane(const Vector4d& world_clip_plane_equation) override
        {
                world_clip_plane_equation_ = world_clip_plane_equation;
                buffer_set_clip_plane();
        }

        UpdateChanges update(const volume::Reading<3>& volume_object) override
        {
                const volume::Updates updates = volume_object.updates(&version_);
                if (updates.none())
                {
                        return {};
                }

                UpdateChanges update_changes;

                static_assert(volume::Updates().size() == 11);

                static constexpr volume::Updates PARAMETERS_UPDATE(
                        (1ull << volume::UPDATE_COLOR) | (1ull << volume::UPDATE_LEVELS)
                        | (1ull << volume::UPDATE_ISOVALUE) | (1ull << volume::UPDATE_ISOSURFACE)
                        | (1ull << volume::UPDATE_ISOSURFACE_ALPHA)
                        | (1ull << volume::UPDATE_VOLUME_ALPHA_COEFFICIENT));

                static constexpr volume::Updates LIGHTING_UPDATE(
                        (1ull << volume::UPDATE_AMBIENT) | (1ull << volume::UPDATE_METALNESS)
                        | (1ull << volume::UPDATE_ROUGHNESS));

                bool size_changed = false;

                if (updates[volume::UPDATE_IMAGE])
                {
                        set_transfer_function();
                        set_image(volume_object.volume().image, &size_changed);
                        create_descriptor_sets();
                        update_changes.command_buffers = true;
                }

                if ((updates & PARAMETERS_UPDATE).any())
                {
                        buffer_set_parameters(
                                volume_object.level_min(), volume_object.level_max(),
                                volume_object.volume_alpha_coefficient(), volume_object.isosurface_alpha(),
                                volume_object.isosurface(), volume_object.isovalue(), volume_object.color());
                }

                if ((updates & LIGHTING_UPDATE).any())
                {
                        buffer_set_lighting(
                                volume_object.ambient(), volume_object.metalness(), volume_object.roughness());
                }

                if (size_changed || updates[volume::UPDATE_MATRICES])
                {
                        object_normal_to_world_normal_matrix_ =
                                volume_object.matrix().top_left<3, 3>().inverse().transpose();
                        texture_to_world_matrix_ = volume_object.matrix() * volume_object.volume().matrix;
                        gradient_h_ = gradient_h(texture_to_world_matrix_, *image_);

                        buffer_set_coordinates();
                }

                return update_changes;
        }

public:
        Impl(const vulkan::Device& device,
             const std::vector<uint32_t>& graphics_family_indices,
             const vulkan::CommandPool& transfer_command_pool,
             const vulkan::Queue& transfer_queue,
             std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts,
             VkSampler image_sampler,
             VkSampler transfer_function_sampler)
                : device_(device),
                  family_indices_(sort_and_unique(
                          merge<std::vector<uint32_t>>(graphics_family_indices, transfer_queue.family_index()))),
                  transfer_command_pool_(transfer_command_pool),
                  transfer_queue_(transfer_queue),
                  buffer_(device_, graphics_family_indices, {transfer_queue.family_index()}),
                  image_layouts_(std::move(image_layouts)),
                  image_sampler_(image_sampler),
                  transfer_function_sampler_(transfer_function_sampler)
        {
                ASSERT(transfer_command_pool.family_index() == transfer_queue.family_index());
        }
};
}

std::unique_ptr<VolumeObject> create_volume_object(
        const vulkan::Device& device,
        const std::vector<uint32_t>& graphics_family_indices,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts,
        VkSampler image_sampler,
        VkSampler transfer_function_sampler)
{
        return std::make_unique<Impl>(
                device, graphics_family_indices, transfer_command_pool, transfer_queue, std::move(image_layouts),
                image_sampler, transfer_function_sampler);
}
}
