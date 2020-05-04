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

#pragma once

#include "shader/volume.h"

#include <src/model/volume_object.h>
#include <src/numerical/matrix.h>
#include <src/vulkan/descriptor.h>
#include <src/vulkan/objects.h>

#include <memory>

namespace gpu::renderer
{
class VolumeObject final
{
        class Volume;

        std::unique_ptr<Volume> m_volume;

public:
        VolumeObject(
                const vulkan::Device& device,
                const vulkan::CommandPool& graphics_command_pool,
                const vulkan::Queue& graphics_queue,
                const vulkan::CommandPool& transfer_command_pool,
                const vulkan::Queue& transfer_queue,
                const std::function<VolumeImageMemory(const VolumeInfo&)>& create_descriptor_sets);

        ~VolumeObject();

        const VkDescriptorSet& descriptor_set(VkDescriptorSetLayout descriptor_set_layout) const;

        void set_matrix_and_clip_plane(const mat4& vp_matrix, const std::optional<vec4>& world_clip_plane_equation);
        void set_clip_plane(const vec4& world_clip_plane_equation);

        void update(
                const std::unordered_set<volume::Update>& updates,
                const volume::VolumeObject<3>& volume_object,
                bool* update_command_buffers);
};
}
