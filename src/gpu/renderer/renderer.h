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

#pragma once

#include "renderer_command.h"

#include <src/gpu/render_buffers.h>
#include <src/model/mesh_object.h>
#include <src/model/volume_object.h>
#include <src/numerical/region.h>
#include <src/vulkan/buffers.h>
#include <src/vulkan/instance.h>

#include <memory>

namespace ns::gpu::renderer
{
struct Renderer
{
        static vulkan::DeviceFeatures required_device_features();

        virtual ~Renderer() = default;

        virtual void exec(Command&& command) = 0;

        virtual void object_update(const mesh::MeshObject<3>& object) = 0;
        virtual void object_update(const volume::VolumeObject<3>& object) = 0;
        virtual void object_delete(ObjectId id) = 0;
        virtual void object_delete_all() = 0;

        virtual VkSemaphore draw(
                const vulkan::Queue& graphics_queue_1,
                const vulkan::Queue& graphics_queue_2,
                unsigned index) const = 0;

        virtual bool empty() const = 0;

        virtual void create_buffers(
                RenderBuffers3D* render_buffers,
                const vulkan::ImageWithMemory* objects,
                const Region<2, int>& viewport) = 0;
        virtual void delete_buffers() = 0;
};

std::unique_ptr<Renderer> create_renderer(
        const vulkan::VulkanInstance* instance,
        const vulkan::CommandPool* graphics_command_pool,
        const vulkan::Queue* graphics_queue,
        const vulkan::CommandPool* transfer_command_pool,
        const vulkan::Queue* transfer_queue,
        bool sample_shading,
        bool sampler_anisotropy);
}
