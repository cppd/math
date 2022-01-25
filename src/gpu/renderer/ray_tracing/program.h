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

#include <src/vulkan/buffers.h>
#include <src/vulkan/device.h>
#include <src/vulkan/objects.h>

#include <memory>
#include <vector>

namespace ns::gpu::renderer
{
class RayTracingProgram final
{
        vulkan::handle::DescriptorSetLayout descriptor_set_layout_;
        vulkan::handle::PipelineLayout pipeline_layout_;
        vulkan::handle::Pipeline pipeline_;

        std::unique_ptr<vulkan::BufferWithMemory> raygen_shader_binding_table_buffer_;
        std::unique_ptr<vulkan::BufferWithMemory> miss_shader_binding_table_buffer_;
        std::unique_ptr<vulkan::BufferWithMemory> hit_shader_binding_table_buffer_;

        VkStridedDeviceAddressRegionKHR raygen_shader_binding_table_;
        VkStridedDeviceAddressRegionKHR miss_shader_binding_table_;
        VkStridedDeviceAddressRegionKHR hit_shader_binding_table_;
        VkStridedDeviceAddressRegionKHR callable_shader_binding_table_;

        void create(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices);

public:
        explicit RayTracingProgram(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices);

        RayTracingProgram(const RayTracingProgram&) = delete;
        RayTracingProgram& operator=(const RayTracingProgram&) = delete;
        RayTracingProgram& operator=(RayTracingProgram&&) = delete;

        RayTracingProgram(RayTracingProgram&&) = default;
        ~RayTracingProgram() = default;

        VkPipeline pipeline() const;

        VkDescriptorSetLayout descriptor_set_layout() const;
        static std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings();

        VkPipelineLayout pipeline_layout() const;

        const VkStridedDeviceAddressRegionKHR& raygen_shader_binding_table() const;
        const VkStridedDeviceAddressRegionKHR& miss_shader_binding_table() const;
        const VkStridedDeviceAddressRegionKHR& hit_shader_binding_table() const;
        const VkStridedDeviceAddressRegionKHR& callable_shader_binding_table() const;
};
}
