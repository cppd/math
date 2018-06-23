/*
Copyright (C) 2017, 2018 Topological Manifold

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

#if defined(VULKAN_FOUND)

#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace vulkan
{
std::string overview();
std::string overview_physical_devices(VkInstance instance);

class Instance
{
        VkInstance m_instance = VK_NULL_HANDLE;

        void create(int api_version_major, int api_version_minor, std::vector<const char*> required_extensions,
                    const std::vector<const char*>& required_validation_layers);
        void destroy() noexcept;
        void move(Instance* from) noexcept;

public:
        Instance(int api_version_major, int api_version_minor, const std::vector<const char*>& required_extensions,
                 const std::vector<const char*>& required_validation_layers);
        ~Instance();

        Instance(const Instance&) = delete;
        Instance& operator=(const Instance&) = delete;

        Instance(Instance&&) noexcept;
        Instance& operator=(Instance&&) noexcept;

        operator VkInstance() const;
};

class DebugReportCallback
{
        VkInstance m_instance = VK_NULL_HANDLE;
        VkDebugReportCallbackEXT m_callback = VK_NULL_HANDLE;

        void create(VkInstance instance);
        void destroy() noexcept;
        void move(DebugReportCallback* from) noexcept;

public:
        DebugReportCallback(VkInstance instance);
        ~DebugReportCallback();

        DebugReportCallback(const DebugReportCallback&) = delete;
        DebugReportCallback& operator=(const DebugReportCallback&) = delete;

        DebugReportCallback(DebugReportCallback&&) noexcept;
        DebugReportCallback& operator=(DebugReportCallback&&) noexcept;

        operator VkDebugReportCallbackEXT() const;
};

class Device
{
        VkDevice m_device = VK_NULL_HANDLE;

        void create(VkPhysicalDevice physical_device, const std::vector<unsigned>& family_indices,
                    const std::vector<const char*>& required_extensions,
                    const std::vector<const char*>& required_validation_layers);
        void destroy() noexcept;
        void move(Device* from) noexcept;

public:
        Device();
        Device(VkPhysicalDevice physical_device, const std::vector<unsigned>& family_indices,
               const std::vector<const char*>& required_extensions, const std::vector<const char*>& required_validation_layers);
        ~Device();

        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        Device(Device&&) noexcept;
        Device& operator=(Device&&) noexcept;

        operator VkDevice() const;
};

class SurfaceKHR
{
        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;

        void create(VkInstance instance, const std::function<VkSurfaceKHR(VkInstance)>& create_surface);
        void destroy() noexcept;
        void move(SurfaceKHR* from) noexcept;

public:
        SurfaceKHR(VkInstance instance, const std::function<VkSurfaceKHR(VkInstance)>& create_surface);
        ~SurfaceKHR();

        SurfaceKHR(const SurfaceKHR&) = delete;
        SurfaceKHR& operator=(const SurfaceKHR&) = delete;

        SurfaceKHR(SurfaceKHR&&) noexcept;
        SurfaceKHR& operator=(SurfaceKHR&&) noexcept;

        operator VkSurfaceKHR() const;
};

class VulkanInstance
{
        Instance m_instance;
        std::optional<DebugReportCallback> m_callback;
        SurfaceKHR m_surface;
        VkPhysicalDevice m_physical_device = VK_NULL_HANDLE;
        Device m_device;
        VkQueue m_graphics_queue = VK_NULL_HANDLE;
        VkQueue m_compute_queue = VK_NULL_HANDLE;
        VkQueue m_presentation_queue = VK_NULL_HANDLE;

public:
        VulkanInstance(int api_version_major, int api_version_minor, const std::vector<const char*>& required_instance_extensions,
                       const std::vector<const char*>& required_device_extensions,
                       const std::vector<const char*>& required_validation_layers,
                       const std::function<VkSurfaceKHR(VkInstance)>& create_surface);
        operator VkInstance() const;
};
}

#endif
