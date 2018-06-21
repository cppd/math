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

#include "application/application_name.h"
#include "com/error.h"
#include "com/log.h"
#include "com/print.h"

#include <algorithm>
#include <array>
#include <sstream>
#include <unordered_set>
#include <vector>

namespace
{
std::string api_version_to_string(uint32_t api_version)
{
        std::ostringstream oss;
        oss << VK_VERSION_MAJOR(api_version) << "." << VK_VERSION_MINOR(api_version) << "." << VK_VERSION_PATCH(api_version);
        return oss.str();
}

std::array<std::string, 2> return_code_strings(const VkResult& code)
{
        switch (code)
        {
        case VK_SUCCESS:
                return {"VK_SUCCESS", "Command successfully completed"};
        case VK_NOT_READY:
                return {"VK_NOT_READY", "A fence or query has not yet completed"};
        case VK_TIMEOUT:
                return {"VK_TIMEOUT", "A wait operation has not completed in the specified time"};
        case VK_EVENT_SET:
                return {"VK_EVENT_SET", "An event is signaled"};
        case VK_EVENT_RESET:
                return {"VK_EVENT_RESET", "An event is unsignaled"};
        case VK_INCOMPLETE:
                return {"VK_INCOMPLETE", "A return array was too small for the result"};
        case VK_ERROR_OUT_OF_HOST_MEMORY:
                return {"VK_ERROR_OUT_OF_HOST_MEMORY", "A host memory allocation has failed"};
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                return {"VK_ERROR_OUT_OF_DEVICE_MEMORY", "A device memory allocation has failed"};
        case VK_ERROR_INITIALIZATION_FAILED:
                return {"VK_ERROR_INITIALIZATION_FAILED",
                        "Initialization of an object could not be completed for implementation-specific reasons"};
        case VK_ERROR_DEVICE_LOST:
                return {"VK_ERROR_DEVICE_LOST", "The logical or physical device has been lost"};
        case VK_ERROR_MEMORY_MAP_FAILED:
                return {"VK_ERROR_MEMORY_MAP_FAILED", "Mapping of a memory object has failed"};
        case VK_ERROR_LAYER_NOT_PRESENT:
                return {"VK_ERROR_LAYER_NOT_PRESENT", "A requested layer is not present or could not be loaded"};
        case VK_ERROR_EXTENSION_NOT_PRESENT:
                return {"VK_ERROR_EXTENSION_NOT_PRESENT", "A requested extension is not supported"};
        case VK_ERROR_FEATURE_NOT_PRESENT:
                return {"VK_ERROR_FEATURE_NOT_PRESENT", "A requested feature is not supported"};
        case VK_ERROR_INCOMPATIBLE_DRIVER:
                return {"VK_ERROR_INCOMPATIBLE_DRIVER",
                        "The requested version of Vulkan is not supported by the driver or is otherwise incompatible"
                        " for implementation-specific reasons"};
        case VK_ERROR_TOO_MANY_OBJECTS:
                return {"VK_ERROR_TOO_MANY_OBJECTS", "Too many objects of the type have already been created"};
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
                return {"VK_ERROR_FORMAT_NOT_SUPPORTED", "A requested format is not supported on this device"};
        case VK_ERROR_FRAGMENTED_POOL:
                return {"VK_ERROR_FRAGMENTED_POOL", "A pool allocation has failed due to fragmentation of the pool’s memory"};
        case VK_ERROR_OUT_OF_POOL_MEMORY:
                return {"VK_ERROR_OUT_OF_POOL_MEMORY", "A pool memory allocation has failed"};
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
                return {"VK_ERROR_INVALID_EXTERNAL_HANDLE", "An external handle is not a valid handle of the specified type"};
        case VK_ERROR_SURFACE_LOST_KHR:
                return {"VK_ERROR_SURFACE_LOST_KHR", "A surface is no longer available"};
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                return {"VK_ERROR_NATIVE_WINDOW_IN_USE_KHR",
                        "The requested window is already in use by Vulkan or another API in a manner which prevents"
                        " it from being used again"};
        case VK_SUBOPTIMAL_KHR:
                return {"VK_SUBOPTIMAL_KHR",
                        "A swapchain no longer matches the surface properties exactly, but can still be used to present"
                        " to the surface successfully"};
        case VK_ERROR_OUT_OF_DATE_KHR:
                return {"VK_ERROR_OUT_OF_DATE_KHR",
                        "A surface has changed in such a way that it is no longer compatible with the swapchain, and further"
                        " presentation requests using the swapchain will fail"};
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
                return {"VK_ERROR_INCOMPATIBLE_DISPLAY_KHR",
                        "The display used by a swapchain does not use the same presentable image layout, or is incompatible"
                        " in a way that prevents sharing an image"};
        case VK_ERROR_VALIDATION_FAILED_EXT:
                return {"VK_ERROR_VALIDATION_FAILED_EXT", ""};
        case VK_ERROR_INVALID_SHADER_NV:
                return {"VK_ERROR_INVALID_SHADER_NV", ""};
        case VK_ERROR_FRAGMENTATION_EXT:
                return {"VK_ERROR_FRAGMENTATION_EXT", ""};
        case VK_ERROR_NOT_PERMITTED_EXT:
                return {"VK_ERROR_NOT_PERMITTED_EXT", ""};
        case VK_RESULT_RANGE_SIZE:
                return {"VK_RESULT_RANGE_SIZE", ""};
        case VK_RESULT_MAX_ENUM:
                return {"VK_RESULT_MAX_ENUM", ""};
        }
        return {"Unknown Vulkan return code " + to_string(static_cast<long long>(code)), ""};
}

std::string return_code_string(const std::string& function_name, const VkResult& code)
{
        std::array<std::string, 2> strings = return_code_strings(code);

        std::string result;

        if (function_name.size() > 0)
        {
                result += function_name + ".";
        }

        for (const std::string& s : strings)
        {
                if (s.size() > 0)
                {
                        result += " " + s + ".";
                }
        }

        if (result.size() > 0)
        {
                return result;
        }

        static_assert(sizeof(VkResult) <= sizeof(long long));
        return "Vulkan Return Code " + to_string(static_cast<long long>(code));
}

// clang-format 6 неправильно форматирует, если [[noreturn]] поставить перед функцией
void vulkan_function_error[[noreturn]](const std::string& function_name, const VkResult& code)
{
        error("Vulkan Error. " + return_code_string(function_name, code));
}

std::unordered_set<std::string> supported_extensions()
{
        uint32_t extension_count;
        VkResult result;

        result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateInstanceExtensionProperties", result);
        }

        if (extension_count < 1)
        {
                return {};
        }

        std::vector<VkExtensionProperties> extensions(extension_count);

        result = vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateInstanceExtensionProperties", result);
        }

        std::unordered_set<std::string> extension_set;

        for (const VkExtensionProperties& e : extensions)
        {
                extension_set.emplace(e.extensionName);
        }

        return extension_set;
}

std::unordered_set<std::string> supported_validation_layers()
{
        uint32_t layer_count;
        VkResult result;

        result = vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateInstanceLayerProperties", result);
        }

        if (layer_count < 1)
        {
                return {};
        }

        std::vector<VkLayerProperties> layers(layer_count);

        result = vkEnumerateInstanceLayerProperties(&layer_count, layers.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateInstanceLayerProperties", result);
        }

        std::unordered_set<std::string> layer_set;

        for (const VkLayerProperties& l : layers)
        {
                layer_set.emplace(l.layerName);
        }

        return layer_set;
}

uint32_t supported_api_version()
{
        if (!vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"))
        {
                return VK_MAKE_VERSION(1, 0, 0);
        }

        uint32_t api_version;
        VkResult result = vkEnumerateInstanceVersion(&api_version);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateInstanceVersion", result);
        }

        return api_version;
}

std::vector<VkPhysicalDevice> physical_devices(VkInstance instance)
{
        uint32_t device_count;
        VkResult result;

        result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumeratePhysicalDevices", result);
        }

        if (device_count < 1)
        {
                error("No Vulkan device found");
        }

        std::vector<VkPhysicalDevice> devices(device_count);

        result = vkEnumeratePhysicalDevices(instance, &device_count, devices.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumeratePhysicalDevices", result);
        }

        return devices;
}

std::vector<VkQueueFamilyProperties> queue_families(VkPhysicalDevice device)
{
        uint32_t queue_family_count;

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

        if (queue_family_count < 1)
        {
                return {};
        }

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);

        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

        return queue_families;
}

void check_extension_support(const std::vector<const char*>& required_extensions)
{
        if (required_extensions.empty())
        {
                return;
        }

        const std::unordered_set<std::string> extension_set = supported_extensions();

        for (std::string e : required_extensions)
        {
                if (extension_set.count(e) < 1)
                {
                        error("Vulkan extension " + e + " is not supported");
                }
        }
}

void check_validation_layer_support(const std::vector<const char*>& required_layers)
{
        if (required_layers.empty())
        {
                return;
        }

        const std::unordered_set<std::string> layer_set = supported_validation_layers();

        for (std::string l : required_layers)
        {
                if (layer_set.count(l) < 1)
                {
                        error("Vulkan validation layer " + l + " is not supported");
                }
        }
}

void check_api_version(uint32_t required_api_version)
{
        uint32_t api_version = supported_api_version();

        if (required_api_version > api_version)
        {
                error("Vulkan API version " + api_version_to_string(required_api_version) + " is not supported. Supported " +
                      api_version_to_string(api_version) + ".");
        }
}

struct FoundPhysicalDevice
{
        const VkPhysicalDevice physical_device;
        const unsigned graphics_family_index;
        const unsigned compute_family_index;
};

FoundPhysicalDevice find_physical_device(VkInstance instance, int api_version_major, int api_version_minor)
{
        const uint32_t required_api_version = VK_MAKE_VERSION(api_version_major, api_version_minor, 0);

        for (const VkPhysicalDevice& device : physical_devices(instance))
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

                unsigned index = 0;
                unsigned graphics_family_index = 0;
                unsigned compute_family_index = 0;
                bool graphics_found = false;
                bool compute_found = false;
                for (const VkQueueFamilyProperties& p : queue_families(device))
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

                        if (graphics_found && compute_found)
                        {
                                break;
                        }

                        ++index;
                }

                if (!graphics_found || !compute_found)
                {
                        continue;
                }

                return {device, graphics_family_index, compute_family_index};
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
std::string overview()
{
        static constexpr const char* INDENT = "  ";

        std::string s;

        s += "API Version";
        try
        {
                s += "\n";
                s += INDENT;
                s += api_version_to_string(supported_api_version());
        }
        catch (std::exception& e)
        {
                s += "\n";
                s += INDENT;
                s += e.what();
        }

        s += "\n";
        s += "Extensions";
        try
        {
                std::unordered_set<std::string> extensions(supported_extensions());
                std::vector<std::string> extension_vector(extensions.begin(), extensions.end());
                std::sort(extension_vector.begin(), extension_vector.end());
                for (const std::string& extension : extension_vector)
                {
                        s += "\n";
                        s += INDENT;
                        s += extension;
                }
        }
        catch (std::exception& e)
        {
                s += "\n";
                s += INDENT;
                s += e.what();
        }

        s += "\n";
        s += "Validation Layers";
        try
        {
                std::unordered_set<std::string> layers(supported_validation_layers());
                std::vector<std::string> layer_vector(layers.begin(), layers.end());
                std::sort(layer_vector.begin(), layer_vector.end());
                for (const std::string& layer : layer_vector)
                {
                        s += "\n";
                        s += INDENT;
                        s += layer;
                }
        }
        catch (std::exception& e)
        {
                s += "\n";
                s += INDENT;
                s += e.what();
        }

        return s;
}

std::string overview_physical_devices(VkInstance instance)
{
        const std::string INDENT = "  ";

        std::string indent;

        std::string s;

        s += "Physical Devices";

        for (const VkPhysicalDevice& device : physical_devices(instance))
        {
                VkPhysicalDeviceProperties properties;
                VkPhysicalDeviceFeatures features;
                vkGetPhysicalDeviceProperties(device, &properties);
                vkGetPhysicalDeviceFeatures(device, &features);

                indent = "\n" + INDENT;

                s += indent + properties.deviceName;

                indent = "\n" + INDENT + INDENT;

                if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                        s += indent + "Discrete GPU";
                }
                else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                {
                        s += indent + "Integrated GPU";
                }
                else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
                {
                        s += indent + "Virtual GPU";
                }
                else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
                {
                        s += indent + "CPU";
                }
                else
                {
                        s += indent + "Unknown Device Type";
                }

                s += indent;
                s += "API Version " + api_version_to_string(properties.apiVersion);

                s += indent;
                s += "QueueFamilies";

                for (const VkQueueFamilyProperties& p : queue_families(device))
                {
                        indent = "\n" + INDENT + INDENT + INDENT;

                        s += indent + "Family";

                        indent = "\n" + INDENT + INDENT + INDENT + INDENT;

                        s += indent;
                        s += "queue count: " + to_string(p.queueCount);

                        if (p.queueCount < 1)
                        {
                                continue;
                        }

                        if (p.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                        {
                                s += indent + "graphics";
                        }
                        if (p.queueFlags & VK_QUEUE_COMPUTE_BIT)
                        {
                                s += indent + "compute";
                        }
                        if (p.queueFlags & VK_QUEUE_TRANSFER_BIT)
                        {
                                s += indent + "transfer";
                        }
                        if (p.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                        {
                                s += indent + "sparse_binding";
                        }
                        if (p.queueFlags & VK_QUEUE_PROTECTED_BIT)
                        {
                                s += indent + "protected";
                        }
                }
        }

        return s;
}

void Instance::create(int api_version_major, int api_version_minor, std::vector<const char*> required_extensions,
                      const std::vector<const char*>& required_validation_layers)
{
        const uint32_t required_api_version = VK_MAKE_VERSION(api_version_major, api_version_minor, 0);

        if (required_validation_layers.size() > 0)
        {
                required_extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        check_api_version(required_api_version);
        check_extension_support(required_extensions);
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

        m_instance = instance;

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

        VkResult result = vkCreateDebugReportCallbackEXT(m_instance, &create_info, nullptr, &m_callback);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkCreateDebugReportCallbackEXT", result);
        }

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

VulkanInstance::VulkanInstance(int api_version_major, int api_version_minor, const std::vector<const char*>& required_extensions,
                               const std::vector<const char*>& required_validation_layers)
        : m_instance(api_version_major, api_version_minor, required_extensions, required_validation_layers),
          m_callback(!required_validation_layers.empty() ? std::make_optional<DebugReportCallback>(m_instance) : std::nullopt)
{
        FoundPhysicalDevice device = find_physical_device(m_instance, api_version_major, api_version_minor);

        m_physical_device = device.physical_device;
        m_device = Device(device.physical_device, {device.graphics_family_index, device.compute_family_index}, {},
                          required_validation_layers);

        constexpr uint32_t queue_index = 0;
        vkGetDeviceQueue(m_device, device.graphics_family_index, queue_index, &m_graphics_queue);
        vkGetDeviceQueue(m_device, device.compute_family_index, queue_index, &m_compute_queue);

        ASSERT(m_physical_device != VK_NULL_HANDLE);
        ASSERT(m_graphics_queue != VK_NULL_HANDLE);
        ASSERT(m_compute_queue != VK_NULL_HANDLE);
}

VulkanInstance::operator VkInstance() const
{
        return m_instance;
}
}

#endif
