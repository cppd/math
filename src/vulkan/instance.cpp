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

#include "instance.h"

#include "create.h"
#include "debug.h"
#include "error.h"
#include "settings.h"

#include <src/com/alg.h>
#include <src/com/log.h>
#include <src/com/merge.h>
#include <src/com/print.h>
#include <src/utility/string/vector.h>

namespace vulkan
{
constexpr uint32_t NO_FAMILY_INDEX = -1;
namespace
{
std::unordered_map<uint32_t, uint32_t> compute_queue_count(
        const std::vector<std::tuple<uint32_t, uint32_t>>& family_index_and_count,
        const std::vector<VkQueueFamilyProperties>& queue_family_properties)
{
        std::unordered_map<uint32_t, uint32_t> queues;
        for (const auto& [index, count] : family_index_and_count)
        {
                if (index != NO_FAMILY_INDEX)
                {
                        queues[index] = std::min(queues[index] + count, queue_family_properties[index].queueCount);
                }
        }
        return queues;
}

std::vector<std::string> merge_required_device_extensions(
        bool with_swapchain,
        const std::vector<std::string>& required_device_extensions)
{
        return with_swapchain ? merge<std::string>(required_device_extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME)
                              : required_device_extensions;
}
}

VulkanInstance::VulkanInstance(
        const std::vector<std::string>& required_instance_extensions,
        const std::vector<std::string>& required_device_extensions,
        const std::vector<PhysicalDeviceFeatures>& required_features,
        const std::vector<PhysicalDeviceFeatures>& optional_features,
        const std::optional<std::function<VkSurfaceKHR(VkInstance)>>& create_surface)
        : m_instance(create_instance(
                  API_VERSION_MAJOR,
                  API_VERSION_MINOR,
                  required_instance_extensions,
                  string_vector(VALIDATION_LAYERS))),
          m_callback(
                  m_instance.validation_layers_enabled() ? std::make_optional(create_debug_report_callback(m_instance))
                                                         : std::nullopt),
          m_surface(create_surface ? std::optional(SurfaceKHR(m_instance, *create_surface)) : std::nullopt),
          //
          m_physical_device(find_physical_device(
                  m_instance,
                  //
                  (create_surface ? static_cast<VkSurfaceKHR>(*m_surface) : VK_NULL_HANDLE),
                  //
                  API_VERSION_MAJOR,
                  API_VERSION_MINOR,
                  //
                  merge_required_device_extensions(create_surface.has_value(), required_device_extensions),
                  //
                  required_features)),
          //
          m_graphics_compute_family_index(
                  m_physical_device.family_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0, 0)),
          m_compute_family_index(m_physical_device.family_index(
                  VK_QUEUE_COMPUTE_BIT,
                  VK_QUEUE_GRAPHICS_BIT,
                  VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)),
          m_transfer_family_index(m_physical_device.family_index(
                  VK_QUEUE_TRANSFER_BIT,
                  VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT,
                  // Наличие VK_QUEUE_GRAPHICS_BIT или VK_QUEUE_COMPUTE_BIT
                  // означает (возможно неявно) наличие VK_QUEUE_TRANSFER_BIT
                  VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)),
          m_presentation_family_index(create_surface ? m_physical_device.presentation_family_index() : NO_FAMILY_INDEX),
          //
          m_device(m_physical_device.create_device(
                  compute_queue_count(
                          {{m_graphics_compute_family_index, GRAPHICS_COMPUTE_QUEUE_COUNT},
                           {m_compute_family_index, COMPUTE_QUEUE_COUNT},
                           {m_transfer_family_index, TRANSFER_QUEUE_COUNT},
                           {m_presentation_family_index, PRESENTATION_QUEUE_COUNT}},
                          m_physical_device.queue_families()),
                  //
                  merge_required_device_extensions(create_surface.has_value(), required_device_extensions),
                  //
                  required_features,
                  //
                  optional_features)),
          //
          m_graphics_compute_command_pool(create_command_pool(m_device, m_graphics_compute_family_index)),
          m_compute_command_pool(create_command_pool(m_device, m_compute_family_index)),
          m_transfer_command_pool(create_transient_command_pool(m_device, m_transfer_family_index))
{
        std::string s;
        std::unordered_map<uint32_t, uint32_t> queue_count;
        uint32_t index;

        index = m_graphics_compute_family_index;
        s += "Graphics compute queues, family index = " + to_string(index);
        for (size_t i = 0; i < m_graphics_compute_queues.size(); ++i)
        {
                m_graphics_compute_queues[i] = m_device.queue(index, queue_count[index]);
                s += "\n  queue = " + to_string(i) + ", device queue = " + to_string(queue_count[index]);
                ++queue_count[index];
                if (queue_count[index] >= m_physical_device.queue_families()[index].queueCount)
                {
                        queue_count[index] = 0;
                }
        }

        s += '\n';
        index = m_compute_family_index;
        s += "Compute queues, family index = " + to_string(index);
        for (size_t i = 0; i < m_compute_queues.size(); ++i)
        {
                m_compute_queues[i] = m_device.queue(index, queue_count[index]);
                s += "\n  queue = " + to_string(i) + ", device queue = " + to_string(queue_count[index]);
                ++queue_count[index];
                if (queue_count[index] >= m_physical_device.queue_families()[index].queueCount)
                {
                        queue_count[index] = 0;
                }
        }

        s += '\n';
        index = m_transfer_family_index;
        s += "Transfer queues, family index = " + to_string(index);
        for (size_t i = 0; i < m_transfer_queues.size(); ++i)
        {
                m_transfer_queues[i] = m_device.queue(index, queue_count[index]);
                s += "\n  queue = " + to_string(i) + ", device queue = " + to_string(queue_count[index]);
                ++queue_count[index];
                if (queue_count[index] >= m_physical_device.queue_families()[index].queueCount)
                {
                        queue_count[index] = 0;
                }
        }

        if (create_surface)
        {
                s += '\n';
                index = m_presentation_family_index;
                s += "Presentation queues, family index = " + to_string(index);
                for (size_t i = 0; i < m_presentation_queues.size(); ++i)
                {
                        m_presentation_queues[i] = m_device.queue(index, queue_count[index]);
                        s += "\n  queue = " + to_string(i) + ", device queue = " + to_string(queue_count[index]);
                        ++queue_count[index];
                        if (queue_count[index] >= m_physical_device.queue_families()[index].queueCount)
                        {
                                queue_count[index] = 0;
                        }
                }
        }

        LOG(s);
}

VulkanInstance::~VulkanInstance()
{
        device_wait_idle_noexcept("the Vulkan instance destructor");
}

void VulkanInstance::device_wait_idle() const
{
        ASSERT(m_device != VK_NULL_HANDLE);

        VkResult result = vkDeviceWaitIdle(m_device);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkDeviceWaitIdle", result);
        }
}

void VulkanInstance::device_wait_idle_noexcept(const char* msg) const noexcept
{
        try
        {
                try
                {
                        device_wait_idle();
                }
                catch (std::exception& e)
                {
                        if (!msg)
                        {
                                error_fatal("No message for the device wait idle function");
                        }
                        std::string s = "Device wait idle exception in " + std::string(msg) + ": " + e.what();
                        LOG(s);
                }
                catch (...)
                {
                        if (!msg)
                        {
                                error_fatal("No message for the device wait idle function");
                        }
                        std::string s = "Device wait idle unknown exception in " + std::string(msg);
                        LOG(s);
                }
        }
        catch (...)
        {
                error_fatal("Exception in the device wait idle exception handlers");
        }
}
}
