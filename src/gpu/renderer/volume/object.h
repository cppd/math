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

#pragma once

#include <src/model/volume_object.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace ns::gpu::renderer
{
class VolumeObject
{
public:
        virtual ~VolumeObject() = default;

        [[nodiscard]] virtual const VkDescriptorSet& descriptor_set(
                VkDescriptorSetLayout descriptor_set_layout) const = 0;

        virtual void set_matrix_and_clip_plane(
                const Matrix4d& vp_matrix,
                const std::optional<Vector4d>& world_clip_plane_equation) = 0;

        virtual void set_matrix_and_clip_plane(
                const Matrix4d& vp_matrix,
                const std::optional<Vector4d>& world_clip_plane_equation,
                const Matrix4d& world_to_shadow_matrix) = 0;

        virtual void set_clip_plane(const Vector4d& world_clip_plane_equation) = 0;

        struct UpdateChanges final
        {
                bool image = false;
        };

        [[nodiscard]] virtual UpdateChanges update(const model::volume::Reading<3>& volume_object) = 0;

        [[nodiscard]] virtual bool is_isosurface() const = 0;
};

std::unique_ptr<VolumeObject> create_volume_object(
        bool ray_tracing,
        const vulkan::Device* device,
        const std::vector<std::uint32_t>& graphics_family_indices,
        const vulkan::CommandPool* transfer_command_pool,
        const vulkan::Queue* transfer_queue,
        std::vector<vulkan::DescriptorSetLayoutAndBindings> image_layouts,
        VkSampler image_sampler,
        VkSampler transfer_function_sampler);
}
