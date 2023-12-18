/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "buffers/volume.h"
#include "shaders/descriptors.h"

#include "../shading_parameters.h"

#include <src/com/alg.h>
#include <src/com/merge.h>
#include <src/image/image.h>
#include <src/vulkan/buffers.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ns::gpu::renderer
{
namespace
{
std::unordered_map<VkDescriptorSetLayout, VolumeImageMemory> create_image_memory(
        const VkDevice device,
        const std::vector<vulkan::DescriptorSetLayoutAndBindings>& image_layouts,
        const vulkan::Buffer& buffer_coordinates,
        const vulkan::Buffer& buffer_volume)
{
        std::unordered_map<VkDescriptorSetLayout, VolumeImageMemory> res;
        for (const vulkan::DescriptorSetLayoutAndBindings& layout : image_layouts)
        {
                VolumeImageMemory memory = VolumeImageMemory(
                        device, layout.descriptor_set_layout, layout.descriptor_set_layout_bindings, buffer_coordinates,
                        buffer_volume);

                res.emplace(layout.descriptor_set_layout, std::move(memory));
        }
        return res;
}

vulkan::ImageWithMemory create_transfer_function(
        const vulkan::Device& device,
        const std::vector<std::uint32_t>& family_indices,
        const vulkan::CommandPool& transfer_command_pool,
        const vulkan::Queue& transfer_queue)
{
        const image::Image<1> transfer_function = volume_transfer_function();

        vulkan::ImageWithMemory image(
                device, family_indices, volume_transfer_function_formats(transfer_function.color_format),
                VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_1D, vulkan::make_extent(transfer_function.size[0]),
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                transfer_command_pool, transfer_queue);

        image.write(
                transfer_command_pool, transfer_queue, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, transfer_function.color_format, transfer_function.pixels);

        return image;
}

class Impl final : public VolumeObject
{
        const bool ray_tracing_;
        const vulkan::Device* const device_;
        const vulkan::CommandPool* const transfer_command_pool_;
        const vulkan::Queue* const transfer_queue_;

        const std::vector<std::uint32_t> family_indices_;

        Matrix4d vp_matrix_ = IDENTITY_MATRIX<4, double>;
        std::optional<Vector4d> world_clip_plane_equation_;

        Matrix3d gradient_to_world_matrix_;
        Matrix4d world_to_texture_matrix_;
        Matrix4d texture_to_world_matrix_;
        Matrix4d world_to_shadow_matrix_ = IDENTITY_MATRIX<4, double>;

        Vector3d gradient_h_;

        VolumeBuffer buffer_;

        vulkan::ImageWithMemory transfer_function_;
        std::unique_ptr<vulkan::ImageWithMemory> image_;
        std::vector<VkFormat> image_formats_;
        bool image_scalar_ = false;
        bool isosurface_ = false;

        const std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts_;
        std::unordered_map<VkDescriptorSetLayout, VolumeImageMemory> image_memory_;

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

        void buffer_set_lighting(const float ambient, const float metalness, const float roughness) const
        {
                buffer_.set_lighting(
                        *transfer_command_pool_, *transfer_queue_, clean_ambient(ambient), clean_metalness(metalness),
                        clean_roughness(roughness));
        }

        void buffer_set_coordinates() const
        {
                const Matrix4d& texture_to_device = vp_matrix_ * texture_to_world_matrix_;
                const geometry::spatial::Hyperplane<3, double>& clip_plane =
                        world_clip_plane_equation_
                                ? volume_clip_plane(*world_clip_plane_equation_, texture_to_world_matrix_)
                                : geometry::spatial::Hyperplane<3, double>(Vector3d(0), 0);

                buffer_.set_coordinates(
                        texture_to_device.inversed(), texture_to_world_matrix_, texture_to_device.row(2), clip_plane,
                        gradient_h_, gradient_to_world_matrix_, world_to_texture_matrix_);

                if (!ray_tracing_)
                {
                        buffer_.set_texture_to_shadow_matrix(world_to_shadow_matrix_ * texture_to_world_matrix_);
                }
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

        void set_memory_image() const
        {
                for (const auto& [layout, memory] : image_memory_)
                {
                        memory.set_image(image_sampler_, image_->image_view().handle());
                }
        }

        void set_memory_transfer_function() const
        {
                for (const auto& [layout, memory] : image_memory_)
                {
                        memory.set_transfer_function(
                                transfer_function_sampler_, transfer_function_.image_view().handle());
                }
        }

        bool set_image(const image::Image<3>& image)
        {
                const auto write = [&](const VkImageLayout image_layout)
                {
                        write_volume_image(
                                image,
                                [&](const image::ColorFormat format, const std::vector<std::byte>& pixels)
                                {
                                        image_->write(
                                                *transfer_command_pool_, *transfer_queue_, image_layout,
                                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, format, pixels);
                                });
                };

                const auto create = [&]
                {
                        image_scalar_ = is_scalar_volume(image.color_format);

                        buffer_set_color_volume(!image_scalar_);

                        image_formats_ = volume_image_formats(image.color_format);

                        image_.reset();
                        image_ = std::make_unique<vulkan::ImageWithMemory>(
                                *device_, family_indices_, image_formats_, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_TYPE_3D,
                                vulkan::make_extent(image.size[0], image.size[1], image.size[2]),
                                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

                        write(VK_IMAGE_LAYOUT_UNDEFINED);

                        set_memory_image();
                };

                if (!image_ || image_->image().extent().width != static_cast<unsigned>(image.size[0])
                    || image_->image().extent().height != static_cast<unsigned>(image.size[1])
                    || image_->image().extent().depth != static_cast<unsigned>(image.size[2]))
                {
                        create();
                        return true;
                }

                if (image_formats_ != volume_image_formats(image.color_format))
                {
                        create();
                        return false;
                }

                write(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                return false;
        }

        void set_texture_to_world_matrix(const Matrix4d& texture_to_world_matrix)
        {
                texture_to_world_matrix_ = texture_to_world_matrix;
                gradient_h_ = volume_gradient_h(texture_to_world_matrix_, image_->image());

                gradient_to_world_matrix_ =
                        texture_to_world_matrix_.top_left<3, 3>() * make_diagonal_matrix(gradient_h_);
                world_to_texture_matrix_ = texture_to_world_matrix_.inversed();

                buffer_set_coordinates();
        }

        const VkDescriptorSet& descriptor_set(const VkDescriptorSetLayout descriptor_set_layout) const override
        {
                const auto iter = image_memory_.find(descriptor_set_layout);
                if (iter != image_memory_.cend())
                {
                        return iter->second.descriptor_set();
                }
                error("Failed to find volume image memory for descriptor set layout");
        }

        void set_matrix_and_clip_plane(
                const Matrix4d& vp_matrix,
                const std::optional<Vector4d>& world_clip_plane_equation) override
        {
                vp_matrix_ = vp_matrix;
                world_clip_plane_equation_ = world_clip_plane_equation;
                buffer_set_coordinates();
        }

        void set_matrix_and_clip_plane(
                const Matrix4d& vp_matrix,
                const std::optional<Vector4d>& world_clip_plane_equation,
                const Matrix4d& world_to_shadow_matrix) override
        {
                ASSERT(!ray_tracing_);
                world_to_shadow_matrix_ = world_to_shadow_matrix;
                set_matrix_and_clip_plane(vp_matrix, world_clip_plane_equation);
        }

        void set_clip_plane(const Vector4d& world_clip_plane_equation) override
        {
                world_clip_plane_equation_ = world_clip_plane_equation;
                buffer_set_clip_plane();
        }

        [[nodiscard]] UpdateChanges update(const model::volume::Reading<3>& volume_object) override
        {
                const model::volume::Updates updates = volume_object.updates(&version_);
                if (updates.none())
                {
                        return {};
                }

                UpdateChanges update_changes;

                static_assert(model::volume::Updates().size() == 12);

                static constexpr model::volume::Updates PARAMETERS_UPDATE(
                        (1ull << model::volume::UPDATE_COLOR) | (1ull << model::volume::UPDATE_LEVELS)
                        | (1ull << model::volume::UPDATE_ISOVALUE) | (1ull << model::volume::UPDATE_ISOSURFACE)
                        | (1ull << model::volume::UPDATE_ISOSURFACE_ALPHA)
                        | (1ull << model::volume::UPDATE_VOLUME_ALPHA_COEFFICIENT));

                static constexpr model::volume::Updates LIGHTING_UPDATE(
                        (1ull << model::volume::UPDATE_AMBIENT) | (1ull << model::volume::UPDATE_METALNESS)
                        | (1ull << model::volume::UPDATE_ROUGHNESS));

                bool size_changed = false;

                if (updates[model::volume::UPDATE_IMAGE])
                {
                        size_changed = set_image(volume_object.volume().image);
                        update_changes.image = true;
                }

                if ((updates & PARAMETERS_UPDATE).any())
                {
                        isosurface_ = volume_object.isosurface();
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

                if (size_changed || updates[model::volume::UPDATE_MATRICES])
                {
                        set_texture_to_world_matrix(volume_object.matrix() * volume_object.volume().matrix);
                }

                return update_changes;
        }

        [[nodiscard]] bool is_isosurface() const override
        {
                return image_scalar_ && isosurface_;
        }

public:
        Impl(const bool ray_tracing,
             const vulkan::Device* const device,
             const std::vector<std::uint32_t>& graphics_family_indices,
             const vulkan::CommandPool* const transfer_command_pool,
             const vulkan::Queue* const transfer_queue,
             std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts,
             const VkSampler image_sampler,
             const VkSampler transfer_function_sampler)
                : ray_tracing_(ray_tracing),
                  device_(device),
                  transfer_command_pool_(transfer_command_pool),
                  transfer_queue_(transfer_queue),
                  family_indices_(sort_and_unique(
                          merge<std::vector<std::uint32_t>>(graphics_family_indices, transfer_queue->family_index()))),
                  buffer_(*device_, graphics_family_indices, {transfer_queue->family_index()}),
                  transfer_function_(create_transfer_function(
                          *device_,
                          family_indices_,
                          *transfer_command_pool_,
                          *transfer_queue_)),
                  image_layouts_(std::move(image_layouts)),
                  image_memory_(create_image_memory(
                          device_->handle(),
                          image_layouts_,
                          buffer_.buffer_coordinates(),
                          buffer_.buffer_volume())),
                  image_sampler_(image_sampler),
                  transfer_function_sampler_(transfer_function_sampler)
        {
                ASSERT(transfer_command_pool->family_index() == transfer_queue->family_index());

                set_memory_transfer_function();
        }
};
}

std::unique_ptr<VolumeObject> create_volume_object(
        const bool ray_tracing,
        const vulkan::Device* const device,
        const std::vector<std::uint32_t>& graphics_family_indices,
        const vulkan::CommandPool* const transfer_command_pool,
        const vulkan::Queue* const transfer_queue,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts,
        const VkSampler image_sampler,
        const VkSampler transfer_function_sampler)
{
        return std::make_unique<Impl>(
                ray_tracing, device, graphics_family_indices, transfer_command_pool, transfer_queue,
                std::move(image_layouts), image_sampler, transfer_function_sampler);
}
}
