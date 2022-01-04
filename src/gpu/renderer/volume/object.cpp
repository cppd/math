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

#include "object.h"

#include "geometry.h"
#include "image.h"

#include "../shading_parameters.h"
#include "buffers/volume.h"
#include "shaders/volume.h"

#include <src/com/alg.h>
#include <src/com/merge.h>
#include <src/image/image.h>
#include <src/vulkan/buffers.h>

#include <unordered_map>

namespace ns::gpu::renderer
{
namespace
{
class Impl final : public VolumeObject
{
        const vulkan::Device* const device_;
        const vulkan::CommandPool* const transfer_command_pool_;
        const vulkan::Queue* const transfer_queue_;

        const std::vector<std::uint32_t> family_indices_;

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
                const float volume_alpha_coefficient,
                float isosurface_alpha,
                const bool isosurface,
                float isovalue,
                const color::Color& color) const
        {
                constexpr float EPS = Limits<float>::epsilon();
                window_min = std::clamp(window_min, 0.0f, 1 - EPS);
                window_max = std::clamp(window_max, window_min + EPS, 1.0f);

                const float window_offset = window_min;
                const float window_scale = 1 / (window_max - window_min);

                isovalue = std::clamp(isovalue, 0.0f, 1.0f);
                isosurface_alpha = std::clamp(isosurface_alpha, 0.0f, 1.0f);

                buffer_.set_parameters(
                        *transfer_command_pool_, *transfer_queue_, window_offset, window_scale,
                        volume_alpha_coefficient, isosurface_alpha, isosurface, isovalue, color.rgb32().clamp(0, 1));
        }

        void buffer_set_lighting(float ambient, float metalness, float roughness) const
        {
                clean_shading_parameters(&ambient, &metalness, &roughness);

                buffer_.set_lighting(*transfer_command_pool_, *transfer_queue_, ambient, metalness, roughness);
        }

        void buffer_set_coordinates() const
        {
                const Matrix4d& mvp = vp_matrix_ * texture_to_world_matrix_;
                const Vector4d& clip_plane =
                        world_clip_plane_equation_
                                ? volume_clip_plane(*world_clip_plane_equation_, texture_to_world_matrix_)
                                : Vector4d(0);

                buffer_.set_coordinates(
                        mvp.inverse(), mvp.row(2), clip_plane, gradient_h_, object_normal_to_world_normal_matrix_);
        }

        void buffer_set_clip_plane() const
        {
                ASSERT(world_clip_plane_equation_);
                buffer_.set_clip_plane(volume_clip_plane(*world_clip_plane_equation_, texture_to_world_matrix_));
        }

        void buffer_set_color_volume(const bool color_volume) const
        {
                buffer_.set_color_volume(*transfer_command_pool_, *transfer_queue_, color_volume);
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
                                *device_, image_sampler_, transfer_function_sampler_, layout.descriptor_set_layout,
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

                const image::Image<1> image = volume_transfer_function();

                transfer_function_.reset();

                transfer_function_ = std::make_unique<vulkan::ImageWithMemory>(
                        *device_, family_indices_, volume_transfer_function_formats(image.color_format),
                        VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_1D, vulkan::make_extent(image.size[0]),
                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                        *transfer_command_pool_, *transfer_queue_);

                transfer_function_->write_pixels(
                        *transfer_command_pool_, *transfer_queue_, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, image.color_format, image.pixels);
        }

        void set_image(const image::Image<3>& image, bool* const size_changed)
        {
                VkImageLayout image_layout;

                if (!image_ || image_formats_ != volume_image_formats(image.color_format)
                    || image_->image().extent().width != static_cast<unsigned>(image.size[0])
                    || image_->image().extent().height != static_cast<unsigned>(image.size[1])
                    || image_->image().extent().depth != static_cast<unsigned>(image.size[2]))
                {
                        *size_changed = true;

                        image_layout = VK_IMAGE_LAYOUT_UNDEFINED;

                        buffer_set_color_volume(!is_scalar_volume(image.color_format));

                        image_formats_ = volume_image_formats(image.color_format);

                        image_.reset();
                        image_ = std::make_unique<vulkan::ImageWithMemory>(
                                *device_, family_indices_, image_formats_, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_3D,
                                vulkan::make_extent(image.size[0], image.size[1], image.size[2]),
                                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, image_layout,
                                *transfer_command_pool_, *transfer_queue_);
                }
                else
                {
                        *size_changed = false;

                        image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }

                write_volume_image(
                        image,
                        [&](const image::ColorFormat format, const std::vector<std::byte>& pixels)
                        {
                                image_->write_pixels(
                                        *transfer_command_pool_, *transfer_queue_, image_layout,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, format, pixels);
                        });
        }

        const VkDescriptorSet& descriptor_set(const VkDescriptorSetLayout descriptor_set_layout) const override
        {
                const auto iter = descriptor_sets_.find(descriptor_set_layout);
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

                static_assert(volume::Updates().size() == 12);

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
                        gradient_h_ = volume_gradient_h(texture_to_world_matrix_, image_->image());

                        buffer_set_coordinates();
                }

                if (updates[volume::UPDATE_VISIBILITY])
                {
                        update_changes.visibility = true;
                }

                return update_changes;
        }

public:
        Impl(const vulkan::Device* const device,
             const std::vector<std::uint32_t>& graphics_family_indices,
             const vulkan::CommandPool* const transfer_command_pool,
             const vulkan::Queue* const transfer_queue,
             std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts,
             const VkSampler image_sampler,
             const VkSampler transfer_function_sampler)
                : device_(device),
                  transfer_command_pool_(transfer_command_pool),
                  transfer_queue_(transfer_queue),
                  family_indices_(sort_and_unique(
                          merge<std::vector<std::uint32_t>>(graphics_family_indices, transfer_queue->family_index()))),
                  buffer_(*device_, graphics_family_indices, {transfer_queue->family_index()}),
                  image_layouts_(std::move(image_layouts)),
                  image_sampler_(image_sampler),
                  transfer_function_sampler_(transfer_function_sampler)
        {
                ASSERT(transfer_command_pool->family_index() == transfer_queue->family_index());
        }
};
}

std::unique_ptr<VolumeObject> create_volume_object(
        const vulkan::Device* const device,
        const std::vector<std::uint32_t>& graphics_family_indices,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts,
        const VkSampler image_sampler,
        const VkSampler transfer_function_sampler)
{
        return std::make_unique<Impl>(
                device, graphics_family_indices, transfer_command_pool, transfer_queue, std::move(image_layouts),
                image_sampler, transfer_function_sampler);
}
}