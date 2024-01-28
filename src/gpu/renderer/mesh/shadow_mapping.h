/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "buffers/shadow_matrices.h"
#include "shaders/descriptors.h"
#include "shaders/program_shadow.h"

#include <src/gpu/renderer/code/code.h>
#include <src/numerical/matrix.h>
#include <src/vulkan/device/device.h>
#include <src/vulkan/objects.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace ns::gpu::renderer
{
class ShadowMapping final
{
        ShadowProgram triangles_program_;
        SharedMemory triangles_shared_memory_;
        vulkan::handle::Sampler sampler_;
        ShadowMatricesBuffer shadow_matrices_buffer_;
        std::unique_ptr<const DepthBuffers> buffers_;
        std::optional<vulkan::handle::Pipeline> render_triangles_pipeline_;
        std::optional<vulkan::handle::CommandBuffers> render_command_buffers_;

public:
        ShadowMapping(
                const vulkan::Device* device,
                const Code& code,
                const vulkan::Buffer& drawing_buffer,
                const std::vector<std::uint32_t>& drawing_family_indices);

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

        void set_shadow_matrices(const numerical::Matrix4d& vp_matrix, const numerical::Matrix4d& world_to_shadow)
                const;

        [[nodiscard]] const vulkan::ImageView& image_view() const;
        [[nodiscard]] VkSampler sampler() const;
        [[nodiscard]] const vulkan::Buffer& shadow_matrices_buffer() const;
        [[nodiscard]] VkDescriptorSetLayout descriptor_set_layout_mesh() const;
        [[nodiscard]] std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_mesh_bindings() const;
        [[nodiscard]] VkCommandBuffer command_buffer(unsigned index) const;
};
}
