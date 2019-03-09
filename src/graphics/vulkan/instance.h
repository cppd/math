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

#pragma once

#include "buffers.h"
#include "device.h"
#include "objects.h"
#include "swapchain.h"

#include "com/container.h"
#include "com/type/detect.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace vulkan
{
class VulkanInstance
{
        const Instance m_instance;
        const std::optional<DebugReportCallback> m_callback;

        const SurfaceKHR m_surface;

        const PhysicalDevice m_physical_device;

        const Device m_device;

        const CommandPool m_graphics_command_pool;
        const VkQueue m_graphics_queue;

        // const CommandPool m_compute_command_pool;
        // const VkQueue m_compute_queue;

        const CommandPool m_transfer_command_pool;
        const VkQueue m_transfer_queue;

        const VkQueue m_presentation_queue;

        const std::vector<uint32_t> m_graphics_and_transfer_family_indices;
        const std::vector<uint32_t> m_graphics_and_presentation_family_indices;

public:
        VulkanInstance(const std::vector<std::string>& required_instance_extensions,
                       const std::vector<std::string>& required_device_extensions,
                       const std::vector<PhysicalDeviceFeatures>& required_features,
                       const std::vector<PhysicalDeviceFeatures>& optional_features,
                       const std::function<VkSurfaceKHR(VkInstance)>& create_surface);

        ~VulkanInstance();

        VulkanInstance(const VulkanInstance&) = delete;
        VulkanInstance(VulkanInstance&&) = delete;
        VulkanInstance& operator=(const VulkanInstance&) = delete;
        VulkanInstance& operator=(VulkanInstance&&) = delete;

        //

        void device_wait_idle() const;
        void device_wait_idle_noexcept(const char* msg) const noexcept;

        //

        VkInstance instance() const noexcept
        {
                return m_instance;
        }

        const Device& device() const noexcept
        {
                return m_device;
        }

        const PhysicalDevice& physical_device() const noexcept
        {
                return m_physical_device;
        }

        VkQueue presentation_queue() const noexcept
        {
                return m_presentation_queue;
        }

        VkQueue graphics_queue() const noexcept
        {
                return m_graphics_queue;
        }

        VkCommandPool graphics_command_pool() const noexcept
        {
                return m_graphics_command_pool;
        }

        //

        Swapchain create_swapchain(VkSurfaceFormatKHR required_surface_format, int preferred_image_count,
                                   vulkan::PresentMode preferred_present_mode) const
        {
                return Swapchain(m_surface, m_device, m_graphics_and_presentation_family_indices, required_surface_format,
                                 preferred_image_count, preferred_present_mode);
        }

        template <typename T>
        BufferWithDeviceLocalMemory create_buffer(VkBufferUsageFlags usage, const T& data) const
        {
                static_assert(is_vector<T> || is_array<T>);
                return BufferWithDeviceLocalMemory(m_device, usage, m_transfer_command_pool, m_transfer_queue,
                                                   m_graphics_and_transfer_family_indices, storage_size(data), data.data());
        }

        template <typename T>
        ColorTexture create_texture(uint32_t width, uint32_t height, const std::vector<T>& rgba_pixels) const
        {
                return ColorTexture(m_device, m_graphics_command_pool, m_graphics_queue, m_transfer_command_pool,
                                    m_transfer_queue, m_graphics_and_transfer_family_indices, width, height, rgba_pixels);
        }

        template <typename T>
        GrayscaleTexture create_grayscale_texture(uint32_t width, uint32_t height, const std::vector<T>& pixels) const
        {
                return GrayscaleTexture(m_device, m_graphics_command_pool, m_graphics_queue, m_transfer_command_pool,
                                        m_transfer_queue, m_graphics_and_transfer_family_indices, width, height, pixels);
        }

        StorageImage create_storage_image(const std::vector<uint32_t>& family_indices, VkFormat format, uint32_t width,
                                          uint32_t height) const
        {
                ASSERT(family_indices.cend() !=
                       std::find(family_indices.cbegin(), family_indices.cend(), m_physical_device.graphics()));
                return StorageImage(m_device, m_graphics_command_pool, m_graphics_queue, family_indices, format, width, height);
        }
};
}
