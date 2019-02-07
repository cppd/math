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

#include "com/log.h"
#include "graphics/vulkan/create.h"
#include "graphics/vulkan/descriptor.h"
#include "graphics/vulkan/pipeline.h"
#include "graphics/vulkan/shader.h"

#include <thread>

constexpr uint32_t SET_NUMBER = 0;

// clang-format off
constexpr std::initializer_list<vulkan::PhysicalDeviceFeatures> REQUIRED_DEVICE_FEATURES =
{
};
// clang-format on

// clang-format off
constexpr uint32_t compute_shader[]
{
#include "ch_debug.comp.spr"
};
// clang-format on

namespace
{
std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings()
{
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        return bindings;
}

class Impl final : public gpgpu_vulkan::ConvexHullCompute
{
        const std::thread::id m_thread_id = std::this_thread::get_id();

        const vulkan::VulkanInstance& m_instance;
        const vulkan::Device& m_device;

        vulkan::ComputeShader m_compute_shader;

        vulkan::DescriptorSetLayout m_descriptor_set_layout;
        vulkan::PipelineLayout m_pipeline_layout;

        vulkan::Pipeline m_pipeline;

        void compute_commands(VkCommandBuffer command_buffer) const override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline);

                vkCmdDispatch(command_buffer, 8, 8, 8);
        }

        void create_buffers(const vulkan::StorageImage& /*objects*/) override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                vulkan::ComputePipelineCreateInfo info;

                info.device = &m_device;
                info.pipeline_layout = m_pipeline_layout;
                info.shader = &m_compute_shader;
                // info.specialization_map_entries;
                // info.specialization_data;

                m_pipeline = create_compute_pipeline(info);
        }

        void delete_buffers() override
        {
                ASSERT(m_thread_id == std::this_thread::get_id());

                //

                m_pipeline = vulkan::Pipeline();
        }

public:
        Impl(const vulkan::VulkanInstance& instance)
                : m_instance(instance),
                  m_device(m_instance.device()),
                  m_compute_shader(m_device, compute_shader, "main"),
                  m_descriptor_set_layout(vulkan::create_descriptor_set_layout(m_device, descriptor_set_layout_bindings())),
                  m_pipeline_layout(vulkan::create_pipeline_layout(m_device, {SET_NUMBER}, {m_descriptor_set_layout}))
        {
        }

        ~Impl() override
        {
                ASSERT(std::this_thread::get_id() == m_thread_id);

                try
                {
                        m_instance.device_wait_idle();
                }
                catch (std::exception& e)
                {
                        LOG(std::string("Device wait idle exception in the Vulkan convex hull compute destructor: ") + e.what());
                }
                catch (...)
                {
                        LOG("Device wait idle unknown exception in the Vulkan convex hull compute destructor");
                }
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
