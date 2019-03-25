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

#include "instance.h"

#include "create.h"
#include "debug.h"
#include "error.h"
#include "settings.h"

#include "com/alg.h"
#include "com/log.h"
#include "com/merge.h"
#include "com/print.h"
#include "com/string/vector.h"

namespace
{
std::unordered_map<uint32_t, uint32_t> queue_families(const std::vector<uint32_t>& family_indices,
                                                      const std::vector<VkQueueFamilyProperties>& queue_families)
{
        std::unordered_map<uint32_t, uint32_t> queues;
        for (uint32_t index : family_indices)
        {
                uint32_t v = queues[index] + 1;
                if (v > queue_families[index].queueCount)
                {
                        error("Too many queues requested, queue family index = " + to_string(index) +
                              ", queue count = " + to_string(queue_families[index].queueCount));
                }
                queues[index] = v;
        }
        return queues;
}
}

namespace vulkan
{
VulkanInstance::VulkanInstance(const std::vector<std::string>& required_instance_extensions,
                               const std::vector<std::string>& required_device_extensions,
                               const std::vector<PhysicalDeviceFeatures>& required_features,
                               const std::vector<PhysicalDeviceFeatures>& optional_features,
                               const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
        : m_instance(create_instance(API_VERSION_MAJOR, API_VERSION_MINOR, required_instance_extensions,
                                     string_vector(VALIDATION_LAYERS))),
          m_callback(m_instance.validation_layers_enabled() ? std::make_optional(create_debug_report_callback(m_instance)) :
                                                              std::nullopt),
          m_surface(m_instance, create_surface),
          //
          m_physical_device(find_physical_device(m_instance, m_surface, API_VERSION_MAJOR, API_VERSION_MINOR,
                                                 merge<std::string>(required_device_extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME),
                                                 required_features)),
          //
          m_graphics_compute_family_index(m_physical_device.family_index(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT, 0, 0)),
          m_transfer_family_index(m_physical_device.family_index(VK_QUEUE_TRANSFER_BIT,
                                                                 VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT,
                                                                 // Наличие VK_QUEUE_GRAPHICS_BIT или VK_QUEUE_COMPUTE_BIT
                                                                 // означает (возможно неявно) наличие VK_QUEUE_TRANSFER_BIT
                                                                 VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)),
          m_presentation_family_index(m_physical_device.presentation_family_index()),
          //
          m_graphics_compute_and_presentation_family_indices(
                  unique_elements(std::vector({m_graphics_compute_family_index, m_presentation_family_index}))),
          m_graphics_compute_and_transfer_family_indices(
                  unique_elements(std::vector({m_graphics_compute_family_index, m_transfer_family_index}))),
          m_graphics_compute_family_indices(unique_elements(std::vector({m_graphics_compute_family_index}))),
          //
          m_device(m_physical_device.create_device(
                  queue_families({m_graphics_compute_family_index, m_transfer_family_index, m_presentation_family_index},
                                 m_physical_device.queue_families()),
                  merge<std::string>(required_device_extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME), required_features,
                  optional_features)),
          //
          m_graphics_compute_command_pool(create_command_pool(m_device, m_graphics_compute_family_index)),
          m_transfer_command_pool(create_transient_command_pool(m_device, m_transfer_family_index))
{
        std::string s;
        std::unordered_map<uint32_t, uint32_t> queue_count;
        uint32_t index;

        index = m_graphics_compute_family_index;
        m_graphics_compute_queue = m_device.queue(index, queue_count[index]);
        s += "Graphics compute family index = " + to_string(index) + ", queue index = " + to_string(queue_count[index]);
        s += "\n";
        ++queue_count[index];

        index = m_transfer_family_index;
        m_transfer_queue = m_device.queue(index, queue_count[index]);
        s += "Transfer family index = " + to_string(index) + ", queue index = " + to_string(queue_count[index]);
        s += "\n";
        ++queue_count[index];

        index = m_presentation_family_index;
        m_presentation_queue = m_device.queue(index, queue_count[index]);
        s += "Presentation family index = " + to_string(index) + ", queue index = " + to_string(queue_count[index]);

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
