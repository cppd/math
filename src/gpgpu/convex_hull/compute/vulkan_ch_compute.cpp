/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "vulkan_ch_compute.h"

#include "com/error.h"
#include "gpgpu/convex_hull/compute/objects/com.h"
#include "gpgpu/convex_hull/compute/objects/vulkan_shader.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/pipeline.h"
#include "graphics/vulkan/shader.h"

#include <optional>
#include <thread>

constexpr uint32_t SET_NUMBER = 0;

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
};
// clang-format on

// clang-format off
constexpr uint32_t debug_shader[]
{
#include "ch_debug.comp.spr"
};
constexpr uint32_t prepare_shader[]
{
#include "ch_prepare.comp.spr"
};
constexpr uint32_t merge_shader[]
{
#include "ch_merge.comp.spr"
};
constexpr uint32_t filter_shader[]
{
#include "ch_filter.comp.spr"
};
// clang-format on

namespace
{
namespace impl
{
using namespace gpgpu_convex_hull_compute_implementation;
using namespace gpgpu_convex_hull_compute_vulkan_implementation;
}

int group_size_prepare(int width, const VkPhysicalDeviceLimits& limits)
{
        return impl::group_size_prepare(width, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                                        limits.maxComputeSharedMemorySize);
}

int group_size_merge(int height, const VkPhysicalDeviceLimits& limits)
{
        return impl::group_size_merge(height, limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupInvocations,
                                      limits.maxComputeSharedMemorySize);
}

class Impl final : public gpgpu_vulkan::ConvexHullCompute
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;

        std::optional<vulkan::BufferWithHostVisibleMemory> m_lines_buffer;
        VkBuffer m_points_buffer = VK_NULL_HANDLE;
        VkBuffer m_point_count_buffer = VK_NULL_HANDLE;

        impl::DebugMemory m_debug_memory;
        impl::DebugConstant m_debug_constant;
        vulkan::ComputeShader m_debug_shader;
        vulkan::PipelineLayout m_debug_pipeline_layout;
        vulkan::Pipeline m_debug_pipeline;

        impl::PrepareMemory m_prepare_memory;
        impl::PrepareConstant m_prepare_constant;
        vulkan::ComputeShader m_prepare_shader;
        vulkan::PipelineLayout m_prepare_pipeline_layout;
        vulkan::Pipeline m_prepare_pipeline;

        impl::MergeMemory m_merge_memory;
        impl::MergeConstant m_merge_constant;
        vulkan::ComputeShader m_merge_shader;
        vulkan::PipelineLayout m_merge_pipeline_layout;
        vulkan::Pipeline m_merge_pipeline;

        impl::FilterMemory m_filter_memory;
        impl::FilterConstant m_filter_constant;
        vulkan::ComputeShader m_filter_shader;
        vulkan::PipelineLayout m_filter_pipeline_layout;
        vulkan::Pipeline m_filter_pipeline;

        void compute_commands(VkCommandBuffer command_buffer) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_debug_pipeline);

                VkDescriptorSet descriptor_set = m_debug_memory.descriptor_set();

                vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_debug_pipeline_layout, SET_NUMBER,
                                        1 /*set count*/, &descriptor_set, 0, nullptr);

                vkCmdDispatch(command_buffer, 1, 1, 1);

                ASSERT(m_points_buffer != VK_NULL_HANDLE);
                {
                        VkBufferMemoryBarrier barrier = {};
                        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.buffer = m_points_buffer;
                        barrier.offset = 0;
                        barrier.size = VK_WHOLE_SIZE;

                        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                             VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1,
                                             &barrier, 0, nullptr);
                }

                ASSERT(m_point_count_buffer != VK_NULL_HANDLE);
                {
                        VkBufferMemoryBarrier barrier = {};
                        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                        barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                        barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier.buffer = m_point_count_buffer;
                        barrier.offset = 0;
                        barrier.size = VK_WHOLE_SIZE;

                        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                             VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 1,
                                             &barrier, 0, nullptr);
                }
        }

        void create_buffers(const vulkan::StorageImage& objects, const vulkan::BufferWithHostVisibleMemory& points_buffer,
                            const vulkan::BufferWithHostVisibleMemory& point_count_buffer) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                ASSERT(points_buffer.size() == (2 * objects.height() + 1) * (2 * sizeof(int32_t)));
                ASSERT(point_count_buffer.size() >= sizeof(int32_t));

                m_lines_buffer.emplace(m_instance.device(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                       2 * objects.height() * sizeof(int32_t));
                m_points_buffer = points_buffer;
                m_point_count_buffer = point_count_buffer;

                const VkPhysicalDeviceLimits& limits = m_instance.physical_device().properties().limits;

                {
                        m_debug_memory.set_object_image(objects);
                        m_debug_memory.set_points(points_buffer);
                        m_debug_memory.set_point_count(point_count_buffer);

                        m_debug_constant.set_local_size_x(4);
                        m_debug_constant.set_local_size_y(1);
                        m_debug_constant.set_local_size_z(1);
                        m_debug_constant.set_buffer_size(1);

                        vulkan::ComputePipelineCreateInfo info;
                        info.device = &m_device;
                        info.pipeline_layout = m_debug_pipeline_layout;
                        info.shader = &m_debug_shader;
                        info.specialization_map_entries = m_debug_constant.entries();
                        info.specialization_data = m_debug_constant.data();
                        info.specialization_data_size = m_debug_constant.size();
                        m_debug_pipeline = create_compute_pipeline(info);
                }
                {
                        m_prepare_memory.set_object_image(objects);
                        m_prepare_memory.set_lines(*m_lines_buffer);

                        m_prepare_constant.set_line_size(objects.height());
                        m_prepare_constant.set_buffer_and_group_size(group_size_prepare(objects.width(), limits));

                        vulkan::ComputePipelineCreateInfo info;
                        info.device = &m_device;
                        info.pipeline_layout = m_prepare_pipeline_layout;
                        info.shader = &m_prepare_shader;
                        info.specialization_map_entries = m_prepare_constant.entries();
                        info.specialization_data = m_prepare_constant.data();
                        info.specialization_data_size = m_prepare_constant.size();
                        m_prepare_pipeline = create_compute_pipeline(info);
                }
                {
                        m_merge_memory.set_lines(*m_lines_buffer);

                        m_merge_constant.set_line_size(objects.height());
                        m_merge_constant.set_local_size_x(group_size_merge(objects.height(), limits));
                        m_merge_constant.set_iteration_count(impl::iteration_count_merge(objects.height()));

                        vulkan::ComputePipelineCreateInfo info;
                        info.device = &m_device;
                        info.pipeline_layout = m_merge_pipeline_layout;
                        info.shader = &m_merge_shader;
                        info.specialization_map_entries = m_merge_constant.entries();
                        info.specialization_data = m_merge_constant.data();
                        info.specialization_data_size = m_merge_constant.size();
                        m_merge_pipeline = create_compute_pipeline(info);
                }
                {
                        m_filter_memory.set_lines(*m_lines_buffer);
                        m_filter_memory.set_points(points_buffer);
                        m_filter_memory.set_point_count(point_count_buffer);

                        m_filter_constant.set_line_size(objects.height());

                        vulkan::ComputePipelineCreateInfo info;
                        info.device = &m_device;
                        info.pipeline_layout = m_filter_pipeline_layout;
                        info.shader = &m_filter_shader;
                        info.specialization_map_entries = m_filter_constant.entries();
                        info.specialization_data = m_filter_constant.data();
                        info.specialization_data_size = m_filter_constant.size();
                        m_filter_pipeline = create_compute_pipeline(info);
                }
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_debug_pipeline = vulkan::Pipeline();

                m_points_buffer = VK_NULL_HANDLE;
                m_point_count_buffer = VK_NULL_HANDLE;
                m_lines_buffer.reset();
        }

public:
        Impl(const vulkan::VulkanInstance& instance)
                : m_instance(instance),
                  m_device(m_instance.device()),
                  //
                  m_debug_memory(m_device),
                  m_debug_shader(m_device, debug_shader, "main"),
                  m_debug_pipeline_layout(
                          vulkan::create_pipeline_layout(m_device, {SET_NUMBER}, {m_debug_memory.descriptor_set_layout()})),

                  m_prepare_memory(m_device),
                  m_prepare_shader(m_device, prepare_shader, "main"),
                  m_prepare_pipeline_layout(
                          vulkan::create_pipeline_layout(m_device, {SET_NUMBER}, {m_prepare_memory.descriptor_set_layout()})),
                  //
                  m_merge_memory(m_device),
                  m_merge_shader(m_device, merge_shader, "main"),
                  m_merge_pipeline_layout(
                          vulkan::create_pipeline_layout(m_device, {SET_NUMBER}, {m_merge_memory.descriptor_set_layout()})),
                  //
                  m_filter_memory(m_device),
                  m_filter_shader(m_device, filter_shader, "main"),
                  m_filter_pipeline_layout(
                          vulkan::create_pipeline_layout(m_device, {SET_NUMBER}, {m_filter_memory.descriptor_set_layout()}))
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                //

                m_instance.device_wait_idle_noexcept("the Vulkan convex hull compute destructor");
        }
};
}

namespace gpgpu_vulkan
{
std::vector<vulkan::PhysicalDeviceFeatures> ConvexHullCompute::required_device_features()
{
        return REQUIRED_DEVICE_FEATURES;
}

std::unique_ptr<ConvexHullCompute> create_convex_hull_compute(const vulkan::VulkanInstance& instance)
{
        return std::make_unique<Impl>(instance);
}
}
