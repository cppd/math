/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "depth_buffers.h"
#include "object.h"

#include "shaders/descriptors.h"
#include "shaders/program_triangles_depth.h"

#include <memory>
#include <optional>
#include <vector>

namespace ns::gpu::renderer
{
class ShadowMapping final
{
        TrianglesDepthProgram triangles_program_;
        SharedMemory triangles_shared_memory_;
        vulkan::handle::Sampler sampler_;
        std::unique_ptr<const DepthBuffers> buffers_;
        std::optional<vulkan::handle::Pipeline> render_triangles_pipeline_;
        std::optional<vulkan::handle::CommandBuffers> render_command_buffers_;

public:
        ShadowMapping(
                const vulkan::Device* device,
                const Code& code,
                const vulkan::Buffer& drawing_buffer,
                const vulkan::Buffer& shadow_matrices_buffer);

        void create_buffers(
                unsigned buffer_count,
                const std::vector<std::uint32_t>& family_indices,
                VkCommandPool graphics_command_pool,
                VkQueue graphics_queue,
                const vulkan::Device& device,
                unsigned width,
                unsigned height,
                double zoom);

        void delete_buffers();

        void create_command_buffers(
                VkDevice device,
                const std::vector<const MeshObject*>& meshes,
                VkCommandPool graphics_command_pool);

        void delete_command_buffers();

        const vulkan::ImageView& image_view() const;
        VkSampler sampler() const;
        VkDescriptorSetLayout descriptor_set_layout_mesh() const;
        std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_mesh_bindings() const;
        std::optional<VkCommandBuffer> command_buffer(unsigned index) const;
};
}
