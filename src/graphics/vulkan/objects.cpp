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

#if defined(VULKAN_FOUND)

#include "objects.h"

#include "common.h"
#include "query.h"

#include "application/application_name.h"
#include "com/error.h"
#include "com/log.h"

namespace
{
struct FoundPhysicalDevice
{
        const VkPhysicalDevice physical_device;
        const unsigned graphics_family_index;
        const unsigned compute_family_index;
        const unsigned presentation_family_index;
};

FoundPhysicalDevice find_physical_device(VkInstance instance, VkSurfaceKHR surface, int api_version_major, int api_version_minor,
                                         const std::vector<const char*>& required_extensions)
{
        const uint32_t required_api_version = VK_MAKE_VERSION(api_version_major, api_version_minor, 0);

        for (const VkPhysicalDevice& device : vulkan::physical_devices(instance))
        {
                VkPhysicalDeviceProperties properties;
                VkPhysicalDeviceFeatures features;
                vkGetPhysicalDeviceProperties(device, &properties);
                vkGetPhysicalDeviceFeatures(device, &features);

                if (properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
                    properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU &&
                    properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU &&
                    properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_CPU)
                {
                        continue;
                }

                if (!features.geometryShader)
                {
                        continue;
                }

                if (!features.tessellationShader)
                {
                        continue;
                }

                if (required_api_version > properties.apiVersion)
                {
                        continue;
                }

                if (!vulkan::device_supports_extensions(device, required_extensions))
                {
                        continue;
                }

                unsigned index = 0;
                unsigned graphics_family_index = 0;
                unsigned compute_family_index = 0;
                unsigned presentation_family_index = 0;
                bool graphics_found = false;
                bool compute_found = false;
                bool presentation_found = false;
                for (const VkQueueFamilyProperties& p : vulkan::queue_families(device))
                {
                        if (p.queueCount < 1)
                        {
                                continue;
                        }

                        if (!graphics_found && (p.queueFlags & VK_QUEUE_GRAPHICS_BIT))
                        {
                                graphics_found = true;
                                graphics_family_index = index;
                        }

                        if (!compute_found && (p.queueFlags & VK_QUEUE_COMPUTE_BIT))
                        {
                                compute_found = true;
                                compute_family_index = index;
                        }

                        if (!presentation_found)
                        {
                                VkBool32 presentation_support;

                                VkResult result =
                                        vkGetPhysicalDeviceSurfaceSupportKHR(device, index, surface, &presentation_support);
                                if (result != VK_SUCCESS)
                                {
                                        vulkan::vulkan_function_error("vkGetPhysicalDeviceSurfaceSupportKHR", result);
                                }

                                if (presentation_support == VK_TRUE)
                                {
                                        presentation_found = true;
                                        presentation_family_index = index;
                                }
                        }

                        if (graphics_found && compute_found && presentation_found)
                        {
                                break;
                        }

                        ++index;
                }

                if (!graphics_found || !compute_found || !presentation_found)
                {
                        continue;
                }

                return {device, graphics_family_index, compute_family_index, presentation_family_index};
        }

        error("Failed to find a suitable Vulkan physical device");
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT /*objectType*/,
                                              uint64_t /*object*/, size_t /*location*/, int32_t /*messageCode*/,
                                              const char* /*pLayerPrefix*/, const char* pMessage, void* /*pUserData*/)
{
        const auto add_to_debug_message = [](std::string* str, const char* text) {
                if (!str->empty())
                {
                        *str += ", ";
                }
                *str += text;
        };

        std::string s;

        if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        {
                add_to_debug_message(&s, "information");
        }
        if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
                add_to_debug_message(&s, "warning");
        }
        if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        {
                add_to_debug_message(&s, "performance warning");
        }
        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
                add_to_debug_message(&s, "error");
        }
        if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        {
                add_to_debug_message(&s, "debug");
        }

        if (s.size() > 0)
        {
                LOG("Validation layer message (" + s + "): " + std::string(pMessage));
        }
        else
        {
                LOG("Validation layer message: " + std::string(pMessage));
        }

        return VK_FALSE;
}
}

namespace vulkan
{
void Instance::create(int api_version_major, int api_version_minor, std::vector<const char*> required_extensions,
                      const std::vector<const char*>& required_validation_layers)
{
        const uint32_t required_api_version = VK_MAKE_VERSION(api_version_major, api_version_minor, 0);

        if (required_validation_layers.size() > 0)
        {
                required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        check_api_version(required_api_version);
        check_instance_extension_support(required_extensions);
        check_validation_layer_support(required_validation_layers);

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = APPLICATION_NAME;
        app_info.applicationVersion = 1;
        app_info.pEngineName = nullptr;
        app_info.engineVersion = 0;
        app_info.apiVersion = required_api_version;

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        if (required_extensions.size() > 0)
        {
                create_info.enabledExtensionCount = required_extensions.size();
                create_info.ppEnabledExtensionNames = required_extensions.data();
        }
        if (required_validation_layers.size() > 0)
        {
                create_info.enabledLayerCount = required_validation_layers.size();
                create_info.ppEnabledLayerNames = required_validation_layers.data();
        }

        VkResult result = vkCreateInstance(&create_info, nullptr, &m_instance);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateInstance", result);
        }

        ASSERT(m_instance != VK_NULL_HANDLE);
}

void Instance::destroy() noexcept
{
        if (m_instance != VK_NULL_HANDLE)
        {
                vkDestroyInstance(m_instance, nullptr);
        }
}

void Instance::move(Instance* from) noexcept
{
        m_instance = from->m_instance;
        from->m_instance = VK_NULL_HANDLE;
}

Instance::Instance(int api_version_major, int api_version_minor, const std::vector<const char*>& required_extensions,
                   const std::vector<const char*>& required_validation_layers)
{
        create(api_version_major, api_version_minor, required_extensions, required_validation_layers);
}

Instance::~Instance()
{
        destroy();
}

Instance::Instance(Instance&& from) noexcept
{
        move(&from);
}

Instance& Instance::operator=(Instance&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

Instance::operator VkInstance() const
{
        return m_instance;
}

//

void DebugReportCallback::create(VkInstance instance)
{
        if (instance == VK_NULL_HANDLE)
        {
                error("No VkInstance for DebugReportCallback");
        }

        VkDebugReportCallbackCreateInfoEXT create_info = {};

        create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;

        create_info.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
        create_info.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
        create_info.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
#if 0
        create_info.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
        create_info.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
#endif

        create_info.pfnCallback = debug_callback;

        VkResult result = vkCreateDebugReportCallbackEXT(instance, &create_info, nullptr, &m_callback);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateDebugReportCallbackEXT", result);
        }

        m_instance = instance;

        ASSERT(m_callback != VK_NULL_HANDLE);
}

void DebugReportCallback::destroy() noexcept
{
        if (m_callback != VK_NULL_HANDLE)
        {
                ASSERT(m_instance != VK_NULL_HANDLE);

                vkDestroyDebugReportCallbackEXT(m_instance, m_callback, nullptr);
        }
}

void DebugReportCallback::move(DebugReportCallback* from) noexcept
{
        m_instance = from->m_instance;
        m_callback = from->m_callback;
        from->m_instance = VK_NULL_HANDLE;
        from->m_callback = VK_NULL_HANDLE;
}

DebugReportCallback::DebugReportCallback(VkInstance instance)
{
        create(instance);
}

DebugReportCallback::~DebugReportCallback()
{
        destroy();
}

DebugReportCallback::DebugReportCallback(DebugReportCallback&& from) noexcept
{
        move(&from);
}

DebugReportCallback& DebugReportCallback::operator=(DebugReportCallback&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

DebugReportCallback::operator VkDebugReportCallbackEXT() const
{
        return m_callback;
}

//

void Device::create(VkPhysicalDevice physical_device, const std::vector<unsigned>& family_indices,
                    const std::vector<const char*>& required_extensions,
                    const std::vector<const char*>& required_validation_layers)
{
        if (family_indices.empty())
        {
                error("No family indices for device creation");
        }

        constexpr float queue_priority = 1;
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        for (unsigned unique_queue_family_index : std::unordered_set<unsigned>(family_indices.cbegin(), family_indices.cend()))
        {
                VkDeviceQueueCreateInfo queue_create_info = {};
                queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queue_create_info.queueFamilyIndex = unique_queue_family_index;
                queue_create_info.queueCount = 1;
                queue_create_info.pQueuePriorities = &queue_priority;

                queue_create_infos.push_back(queue_create_info);
        }

        VkPhysicalDeviceFeatures device_features = {};

        VkDeviceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.queueCreateInfoCount = queue_create_infos.size();
        create_info.pQueueCreateInfos = queue_create_infos.data();
        create_info.pEnabledFeatures = &device_features;
        if (required_extensions.size() > 0)
        {
                create_info.enabledExtensionCount = required_extensions.size();
                create_info.ppEnabledExtensionNames = required_extensions.data();
        }
        if (required_validation_layers.size() > 0)
        {
                create_info.enabledLayerCount = required_validation_layers.size();
                create_info.ppEnabledLayerNames = required_validation_layers.data();
        }

        VkResult result = vkCreateDevice(physical_device, &create_info, nullptr, &m_device);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateDevice", result);
        }

        ASSERT(m_device != VK_NULL_HANDLE);
}

void Device::destroy() noexcept
{
        if (m_device != VK_NULL_HANDLE)
        {
                vkDestroyDevice(m_device, nullptr);
        }
}

void Device::move(Device* from) noexcept
{
        m_device = from->m_device;
        from->m_device = VK_NULL_HANDLE;
}

Device::Device() = default;

Device::Device(VkPhysicalDevice physical_device, const std::vector<unsigned>& family_indices,
               const std::vector<const char*>& required_extensions, const std::vector<const char*>& required_validation_layers)
{
        create(physical_device, family_indices, required_extensions, required_validation_layers);
}

Device::~Device()
{
        destroy();
}

Device::Device(Device&& from) noexcept
{
        move(&from);
}

Device& Device::operator=(Device&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

Device::operator VkDevice() const
{
        return m_device;
}

//

void SurfaceKHR::create(VkInstance instance, const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
{
        if (instance == VK_NULL_HANDLE)
        {
                error("No VkInstance for VkSurfaceKHR creation");
        }

        m_surface = create_surface(instance);

        ASSERT(m_surface != VK_NULL_HANDLE);

        m_instance = instance;
}

void SurfaceKHR::destroy() noexcept
{
        if (m_surface != VK_NULL_HANDLE)
        {
                ASSERT(m_instance != VK_NULL_HANDLE);

                vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        }
}

void SurfaceKHR::move(SurfaceKHR* from) noexcept
{
        m_instance = from->m_instance;
        m_surface = from->m_surface;
        from->m_instance = VK_NULL_HANDLE;
        from->m_surface = VK_NULL_HANDLE;
}

SurfaceKHR::SurfaceKHR(VkInstance instance, const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
{
        create(instance, create_surface);
}

SurfaceKHR::~SurfaceKHR()
{
        destroy();
}

SurfaceKHR::SurfaceKHR(SurfaceKHR&& from) noexcept
{
        move(&from);
}

SurfaceKHR& SurfaceKHR::operator=(SurfaceKHR&& from) noexcept
{
        if (this != &from)
        {
                destroy();
                move(&from);
        }
        return *this;
}

SurfaceKHR::operator VkSurfaceKHR() const
{
        return m_surface;
}

//

VulkanInstance::VulkanInstance(int api_version_major, int api_version_minor,
                               const std::vector<const char*>& required_instance_extensions,
                               const std::vector<const char*>& required_device_extensions,
                               const std::vector<const char*>& required_validation_layers,
                               const std::function<VkSurfaceKHR(VkInstance)>& create_surface)
        : m_instance(api_version_major, api_version_minor, required_instance_extensions, required_validation_layers),
          m_callback(!required_validation_layers.empty() ? std::make_optional<DebugReportCallback>(m_instance) : std::nullopt),
          m_surface(m_instance, create_surface)
{
        FoundPhysicalDevice device =
                find_physical_device(m_instance, m_surface, api_version_major, api_version_minor, required_device_extensions);

        m_physical_device = device.physical_device;

        ASSERT(m_physical_device != VK_NULL_HANDLE);

        m_device = Device(device.physical_device,
                          {device.graphics_family_index, device.compute_family_index, device.presentation_family_index}, {},
                          required_validation_layers);

        constexpr uint32_t queue_index = 0;
        vkGetDeviceQueue(m_device, device.graphics_family_index, queue_index, &m_graphics_queue);
        vkGetDeviceQueue(m_device, device.compute_family_index, queue_index, &m_compute_queue);
        vkGetDeviceQueue(m_device, device.presentation_family_index, queue_index, &m_presentation_queue);

        ASSERT(m_graphics_queue != VK_NULL_HANDLE);
        ASSERT(m_compute_queue != VK_NULL_HANDLE);
        ASSERT(m_presentation_queue != VK_NULL_HANDLE);
}

VulkanInstance::operator VkInstance() const
{
        return m_instance;
}
}

#endif
