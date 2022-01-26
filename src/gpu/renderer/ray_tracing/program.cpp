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

#include "program.h"

#include "descriptors.h"

#include "code/code.h"

#include <src/vulkan/create.h>
#include <src/vulkan/error.h>
#include <src/vulkan/extensions.h>
#include <src/vulkan/pipeline.h>

namespace ns::gpu::renderer
{
namespace
{
VkDeviceAddress buffer_device_address(const VkDevice device, const VkBuffer buffer)
{
        VkBufferDeviceAddressInfo info{};
        info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
        info.buffer = buffer;
        return vkGetBufferDeviceAddress(device, &info);
}
}

std::vector<VkDescriptorSetLayoutBinding> RayTracingProgram::descriptor_set_layout_bindings()
{
        return RayTracingMemory::descriptor_set_layout_bindings();
}

RayTracingProgram::RayTracingProgram(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices)
        : descriptor_set_layout_(vulkan::create_descriptor_set_layout(device, descriptor_set_layout_bindings())),
          pipeline_layout_(
                  vulkan::create_pipeline_layout(device, {RayTracingMemory::set_number()}, {descriptor_set_layout_}))
{
        create(device, family_indices);
}

VkDescriptorSetLayout RayTracingProgram::descriptor_set_layout() const
{
        return descriptor_set_layout_;
}

VkPipelineLayout RayTracingProgram::pipeline_layout() const
{
        return pipeline_layout_;
}

VkPipeline RayTracingProgram::pipeline() const
{
        return pipeline_;
}

void RayTracingProgram::create(const vulkan::Device& device, const std::vector<std::uint32_t>& family_indices)
{
        const vulkan::Shader ray_closest_hit_shader(
                device, code_ray_closest_hit_rchit(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

        const vulkan::Shader ray_generation_shader(device, code_ray_generation_rgen(), VK_SHADER_STAGE_RAYGEN_BIT_KHR);

        const vulkan::Shader ray_miss_shader(device, code_ray_miss_rmiss(), VK_SHADER_STAGE_MISS_BIT_KHR);

        std::vector<const vulkan::Shader*> shaders(3);

        shaders[0] = &ray_closest_hit_shader;
        shaders[1] = &ray_generation_shader;
        shaders[2] = &ray_miss_shader;

        std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_groups(3);

        shader_groups[0] = {};
        shader_groups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        shader_groups[0].generalShader = VK_SHADER_UNUSED_KHR;
        shader_groups[0].closestHitShader = 0;
        shader_groups[0].anyHitShader = VK_SHADER_UNUSED_KHR;
        shader_groups[0].intersectionShader = VK_SHADER_UNUSED_KHR;

        shader_groups[1] = {};
        shader_groups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shader_groups[1].generalShader = 1;
        shader_groups[1].closestHitShader = VK_SHADER_UNUSED_KHR;
        shader_groups[1].anyHitShader = VK_SHADER_UNUSED_KHR;
        shader_groups[1].intersectionShader = VK_SHADER_UNUSED_KHR;

        shader_groups[2] = {};
        shader_groups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        shader_groups[2].generalShader = 2;
        shader_groups[2].closestHitShader = VK_SHADER_UNUSED_KHR;
        shader_groups[2].anyHitShader = VK_SHADER_UNUSED_KHR;
        shader_groups[2].intersectionShader = VK_SHADER_UNUSED_KHR;

        vulkan::RayTracingPipelineCreateInfo info;
        info.device = device;
        info.pipeline_layout = pipeline_layout_;
        info.shaders = &shaders;
        info.shader_groups = &shader_groups;

        pipeline_ = vulkan::create_ray_tracing_pipeline(info);

        //

        const std::size_t handle_size = device.properties().ray_tracing_pipeline->shaderGroupHandleSize;
        constexpr std::size_t GROUP_COUNT = 3;

        std::vector<std::uint8_t> shader_group_handles(handle_size * GROUP_COUNT);

        VULKAN_CHECK(vkGetRayTracingShaderGroupHandlesKHR(
                device, pipeline_, 0, GROUP_COUNT, shader_group_handles.size(), shader_group_handles.data()));

        constexpr VkBufferUsageFlags USAGE_FLAGS =
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;

        raygen_shader_binding_table_buffer_ = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::HOST_VISIBLE, device, family_indices, USAGE_FLAGS, handle_size);
        miss_shader_binding_table_buffer_ = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::HOST_VISIBLE, device, family_indices, USAGE_FLAGS, handle_size);
        hit_shader_binding_table_buffer_ = std::make_unique<vulkan::BufferWithMemory>(
                vulkan::BufferMemoryType::HOST_VISIBLE, device, family_indices, USAGE_FLAGS, handle_size);

        vulkan::BufferMapper(*hit_shader_binding_table_buffer_).write(0, handle_size, shader_group_handles.data());
        vulkan::BufferMapper(*raygen_shader_binding_table_buffer_)
                .write(0, handle_size, shader_group_handles.data() + handle_size);
        vulkan::BufferMapper(*miss_shader_binding_table_buffer_)
                .write(0, handle_size, shader_group_handles.data() + 2 * handle_size);

        raygen_shader_binding_table_.deviceAddress =
                buffer_device_address(device, raygen_shader_binding_table_buffer_->buffer());
        raygen_shader_binding_table_.stride = handle_size;
        raygen_shader_binding_table_.size = handle_size;

        miss_shader_binding_table_.deviceAddress =
                buffer_device_address(device, miss_shader_binding_table_buffer_->buffer());
        miss_shader_binding_table_.stride = handle_size;
        miss_shader_binding_table_.size = handle_size;

        hit_shader_binding_table_.deviceAddress =
                buffer_device_address(device, hit_shader_binding_table_buffer_->buffer());
        hit_shader_binding_table_.stride = handle_size;
        hit_shader_binding_table_.size = handle_size;

        callable_shader_binding_table_ = {};
}

void RayTracingProgram::command_trace_rays(
        const VkCommandBuffer command_buffer,
        const unsigned width,
        const unsigned height,
        const unsigned depth) const
{
        vkCmdTraceRaysKHR(
                command_buffer, &raygen_shader_binding_table_, &miss_shader_binding_table_, &hit_shader_binding_table_,
                &callable_shader_binding_table_, width, height, depth);
}
}
