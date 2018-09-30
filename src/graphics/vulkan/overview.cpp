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

#include "overview.h"

#include "common.h"
#include "device.h"
#include "query.h"
#include "window.h"

#include "com/print.h"

#include <algorithm>

namespace
{
template <typename T>
std::vector<std::string> sorted(const T& s)
{
        std::vector<std::string> res(s.cbegin(), s.cend());
        std::sort(res.begin(), res.end());
        return res;
}
}

namespace vulkan
{
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

        s += "\n";
        s += "Required Window Extensions";
        try
        {
                for (const std::string& extension : sorted(VulkanWindow::instance_extensions()))
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
                        for (const VkQueueFamilyProperties& p : physical_device_queue_families(device))
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
