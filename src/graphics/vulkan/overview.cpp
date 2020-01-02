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

#include "overview.h"

#include "device.h"
#include "error.h"
#include "print.h"
#include "query.h"

#include "com/print.h"
#include "window/vulkan/window.h"

#include <algorithm>
#include <stack>
#include <string_view>
#include <tuple>
#include <vector>

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
                for (const std::string& extension : sorted(Window::instance_extensions()))
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

std::string overview_physical_devices(VkInstance instance, VkSurfaceKHR surface)
{
        struct Node
        {
                std::string name;
                std::vector<size_t> children;
                Node(const std::string_view& s) : name(s)
                {
                }
        };

        std::vector<Node> nodes;

        nodes.emplace_back("Physical Devices");
        size_t physical_devices_node = nodes.size() - 1;

        for (const VkPhysicalDevice& d : physical_devices(instance))
        {
                PhysicalDevice device(d, surface);

                nodes.emplace_back(device.properties().deviceName);
                nodes[physical_devices_node].children.push_back(nodes.size() - 1);

                size_t physical_device_node = nodes.size() - 1;

                nodes.emplace_back(physical_device_type_to_string(device.properties().deviceType));
                nodes[physical_device_node].children.push_back(nodes.size() - 1);

                nodes.emplace_back("API Version " + api_version_to_string(device.properties().apiVersion));
                nodes[physical_device_node].children.push_back(nodes.size() - 1);

                nodes.emplace_back("Extensions");
                nodes[physical_device_node].children.push_back(nodes.size() - 1);

                size_t extension_node = nodes.size() - 1;
                try
                {
                        for (const std::string& e : sorted(device.supported_extensions()))
                        {
                                nodes.emplace_back(e);
                                nodes[extension_node].children.push_back(nodes.size() - 1);
                        }
                }
                catch (std::exception& e)
                {
                        nodes.emplace_back(e.what());
                        nodes[extension_node].children.push_back(nodes.size() - 1);
                }

                nodes.emplace_back("QueueFamilies");
                nodes[physical_device_node].children.push_back(nodes.size() - 1);

                size_t queue_families_node = nodes.size() - 1;
                try
                {
                        std::vector<VkQueueFamilyProperties> families = device.queue_families();

                        for (size_t family_index = 0; family_index < families.size(); ++family_index)
                        {
                                const VkQueueFamilyProperties& p = families[family_index];

                                nodes.emplace_back("Family " + to_string(family_index));
                                nodes[queue_families_node].children.push_back(nodes.size() - 1);

                                size_t queue_family_node = nodes.size() - 1;

                                nodes.emplace_back("queue count: " + to_string(p.queueCount));
                                nodes[queue_family_node].children.push_back(nodes.size() - 1);

                                if (p.queueCount < 1)
                                {
                                        continue;
                                }

                                if (p.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                                {
                                        nodes.emplace_back("graphics");
                                        nodes[queue_family_node].children.push_back(nodes.size() - 1);
                                }
                                if (p.queueFlags & VK_QUEUE_COMPUTE_BIT)
                                {
                                        nodes.emplace_back("compute");
                                        nodes[queue_family_node].children.push_back(nodes.size() - 1);
                                }
                                if (p.queueFlags & VK_QUEUE_TRANSFER_BIT)
                                {
                                        nodes.emplace_back("transfer");
                                        nodes[queue_family_node].children.push_back(nodes.size() - 1);
                                }
                                if (p.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                                {
                                        nodes.emplace_back("sparse binding");
                                        nodes[queue_family_node].children.push_back(nodes.size() - 1);
                                }
                                if (p.queueFlags & VK_QUEUE_PROTECTED_BIT)
                                {
                                        nodes.emplace_back("protected");
                                        nodes[queue_family_node].children.push_back(nodes.size() - 1);
                                }

                                if (device.queue_family_supports_presentation(family_index))
                                {
                                        nodes.emplace_back("presentation");
                                        nodes[queue_family_node].children.push_back(nodes.size() - 1);
                                }
                        }
                }
                catch (std::exception& e)
                {
                        nodes.emplace_back(e.what());
                        nodes[queue_families_node].children.push_back(nodes.size() - 1);
                }
        }

        std::string s;

        std::stack<std::tuple<size_t, unsigned>> stack({{0, 0}});
        while (!stack.empty())
        {
                auto index = std::get<0>(stack.top());
                auto level = std::get<1>(stack.top());
                stack.pop();

                s += '\n' + std::string(level * 2, ' ') + nodes[index].name;

                for (auto iter = nodes[index].children.crbegin(); iter != nodes[index].children.crend(); ++iter)
                {
                        stack.push({*iter, level + 1});
                }
        }

        return s;
}
}
