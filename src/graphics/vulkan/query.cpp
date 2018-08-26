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

#include "query.h"

#include "common.h"

#include "com/error.h"
#include "com/print.h"

#include <algorithm>

namespace
{
std::vector<std::string> sorted(const std::unordered_set<std::string>& s)
{
        std::vector<std::string> res(s.cbegin(), s.cend());
        std::sort(res.begin(), res.end());
        return res;
}
}

namespace vulkan
{
std::unordered_set<std::string> supported_instance_extensions()
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

std::unordered_set<std::string> supported_physical_device_extensions(VkPhysicalDevice physical_device)
{
        uint32_t extension_count;
        VkResult result;

        result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateDeviceExtensionProperties", result);
        }

        if (extension_count < 1)
        {
                return {};
        }

        std::vector<VkExtensionProperties> extensions(extension_count);

        result = vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, extensions.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkEnumerateDeviceExtensionProperties", result);
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

uint32_t supported_instance_api_version()
{
        PFN_vkEnumerateInstanceVersion f =
                reinterpret_cast<PFN_vkEnumerateInstanceVersion>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));

        if (!f)
        {
                return VK_MAKE_VERSION(1, 0, 0);
        }

        uint32_t api_version;
        VkResult result = f(&api_version);
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

void check_instance_extension_support(const std::vector<std::string>& required_extensions)
{
        if (required_extensions.empty())
        {
                return;
        }

        const std::unordered_set<std::string> extension_set = supported_instance_extensions();

        for (const std::string& e : required_extensions)
        {
                if (extension_set.count(e) < 1)
                {
                        error("Vulkan instance extension " + e + " is not supported");
                }
        }
}

void check_validation_layer_support(const std::vector<std::string>& required_layers)
{
        if (required_layers.empty())
        {
                return;
        }

        const std::unordered_set<std::string> layer_set = supported_validation_layers();

        for (const std::string& l : required_layers)
        {
                if (layer_set.count(l) < 1)
                {
                        error("Vulkan validation layer " + l + " is not supported");
                }
        }
}

void check_api_version(uint32_t required_api_version)
{
        uint32_t api_version = supported_instance_api_version();

        if (required_api_version > api_version)
        {
                error("Vulkan API version " + api_version_to_string(required_api_version) + " is not supported. Supported " +
                      api_version_to_string(api_version) + ".");
        }
}

bool device_supports_extensions(VkPhysicalDevice physical_device, const std::vector<std::string>& extensions)
{
        if (extensions.empty())
        {
                return true;
        }

        const std::unordered_set<std::string> extension_set = supported_physical_device_extensions(physical_device);

        for (const std::string& e : extensions)
        {
                if (extension_set.count(e) < 1)
                {
                        return false;
                }
        }

        return true;
}

std::vector<VkSurfaceFormatKHR> surface_formats(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
        uint32_t format_count;
        VkResult result;

        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkGetPhysicalDeviceSurfaceFormatsKHR", result);
        }

        if (format_count < 1)
        {
                return {};
        }

        std::vector<VkSurfaceFormatKHR> formats(format_count);

        result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkGetPhysicalDeviceSurfaceFormatsKHR", result);
        }

        return formats;
}

std::vector<VkPresentModeKHR> present_modes(VkPhysicalDevice physical_device, VkSurfaceKHR surface)
{
        uint32_t mode_count;
        VkResult result;

        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &mode_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkGetPhysicalDeviceSurfacePresentModesKHR", result);
        }

        if (mode_count < 1)
        {
                return {};
        }

        std::vector<VkPresentModeKHR> modes(mode_count);

        result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &mode_count, modes.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkGetPhysicalDeviceSurfacePresentModesKHR", result);
        }

        return modes;
}

std::vector<VkImage> swap_chain_images(VkDevice device, VkSwapchainKHR swap_chain)
{
        uint32_t image_count;
        VkResult result;

        result = vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkGetSwapchainImagesKHR", result);
        }

        if (image_count < 1)
        {
                return {};
        }

        std::vector<VkImage> images(image_count);

        result = vkGetSwapchainImagesKHR(device, swap_chain, &image_count, images.data());
        if (result != VK_SUCCESS)
        {
                vulkan_function_error("vkGetSwapchainImagesKHR", result);
        }

        return images;
}

std::string overview()
{
        constexpr const char* INDENT = "  ";

        std::string s;

        s += "API Version";
        try
        {
                s += "\n";
                s += INDENT;
                s += api_version_to_string(supported_instance_api_version());
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
                for (const std::string& extension : sorted(supported_instance_extensions()))
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
                for (const std::string& layer : sorted(supported_validation_layers()))
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
                s += indent;
                s += properties.deviceName;

                indent = "\n" + INDENT + INDENT;
                s += indent;
                s += physical_device_type_to_string(properties.deviceType);

                indent = "\n" + INDENT + INDENT;
                s += indent;
                s += "API Version " + api_version_to_string(properties.apiVersion);

                indent = "\n" + INDENT + INDENT;
                s += indent;
                s += "Extensions";
                indent = "\n" + INDENT + INDENT + INDENT;
                try
                {
                        for (const std::string& e : sorted(supported_physical_device_extensions(device)))
                        {
                                s += indent;
                                s += e;
                        }
                }
                catch (std::exception& e)
                {
                        indent = "\n" + INDENT + INDENT + INDENT;
                        s += indent;
                        s += e.what();
                }

                indent = "\n" + INDENT + INDENT;
                s += indent;
                s += "QueueFamilies";
                try
                {
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
                catch (std::exception& e)
                {
                        indent = "\n" + INDENT + INDENT + INDENT;
                        s += indent;
                        s += e.what();
                }
        }

        return s;
}
}
