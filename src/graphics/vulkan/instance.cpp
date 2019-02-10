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
#include "com/string/vector.h"

namespace vulkan
{
VulkanInstance::VulkanInstance(const std::vector<std::string>& required_instance_extensions,
                               const std::vector<std::string>& required_device_extensions,
                               const std::vector<PhysicalDeviceFeatures>& required_features,
                               const std::vector<PhysicalDeviceFeatures>& optional_features,
                               const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
        : m_instance(create_instance(API_VERSION_MAJOR, API_VERSION_MINOR, required_instance_extensions, validation_layers())),
          m_callback(!validation_layers().empty() ? std::make_optional(create_debug_report_callback(m_instance)) : std::nullopt),
          m_surface(m_instance, create_surface),
          //
          m_physical_device(find_physical_device(m_instance, m_surface, API_VERSION_MAJOR, API_VERSION_MINOR,
                                                 merge<std::string>(required_device_extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME),
                                                 required_features)),
          m_device(create_device(
                  m_physical_device,
                  {m_physical_device.graphics(), m_physical_device.compute(), m_physical_device.transfer(),
                   m_physical_device.presentation()},
                  merge<std::string>(required_device_extensions, VK_KHR_SWAPCHAIN_EXTENSION_NAME), validation_layers(),
                  make_enabled_device_features(required_features, optional_features, m_physical_device.features()))),
          //
          m_graphics_command_pool(create_command_pool(m_device, m_physical_device.graphics())),
          m_graphics_queue(device_queue(m_device, m_physical_device.graphics(), 0 /*queue_index*/)),
          //
          m_transfer_command_pool(create_transient_command_pool(m_device, m_physical_device.transfer())),
          m_transfer_queue(device_queue(m_device, m_physical_device.transfer(), 0 /*queue_index*/)),
          //
          m_compute_queue(device_queue(m_device, m_physical_device.compute(), 0 /*queue_index*/)),
          //
          m_presentation_queue(device_queue(m_device, m_physical_device.presentation(), 0 /*queue_index*/)),
          //
          m_buffer_family_indices(unique_elements(std::vector({m_physical_device.graphics(), m_physical_device.transfer()}))),
          m_swapchain_family_indices(
                  unique_elements(std::vector({m_physical_device.graphics(), m_physical_device.presentation()}))),
          m_texture_family_indices(unique_elements(std::vector({m_physical_device.graphics(), m_physical_device.transfer()}))),
          m_attachment_family_indices(unique_elements(std::vector({m_physical_device.graphics()}))),
          m_graphics_and_compute_family_indices(
                  unique_elements(std::vector({m_physical_device.graphics(), m_physical_device.compute()})))
{
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
