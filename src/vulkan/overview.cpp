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

#include <src/com/print.h>
#include <src/window/manage.h>

#include <algorithm>
#include <stack>
#include <string_view>
#include <tuple>
#include <vector>

namespace vulkan
{
namespace
{
template <typename T>
std::vector<std::string> sorted(const T& s)
{
        std::vector<std::string> res(s.cbegin(), s.cend());
        std::sort(res.begin(), res.end());
        return res;
}

class Tree
{
        struct Node
        {
                std::string name;
                std::vector<size_t> children;
                explicit Node(const std::string_view& s) : name(s)
                {
                }
        };

        static constexpr size_t ROOT_NODE = 0;
        static constexpr unsigned TEXT_INDENT = 4;

        std::vector<Node> m_nodes;

public:
        Tree()
        {
                m_nodes.emplace_back("");
        }

        size_t add(const std::string& s)
        {
                m_nodes.emplace_back(s);
                m_nodes[ROOT_NODE].children.push_back(m_nodes.size() - 1);
                return m_nodes.size() - 1;
        }

        size_t add(size_t parent, const std::string& s)
        {
                m_nodes.emplace_back(s);
                m_nodes[parent].children.push_back(m_nodes.size() - 1);
                return m_nodes.size() - 1;
        }

        std::string text()
        {
                std::string s;

                std::stack<std::tuple<size_t, unsigned>> stack({{ROOT_NODE, 0}});
                while (!stack.empty())
                {
                        auto index = std::get<0>(stack.top());
                        auto level = std::get<1>(stack.top());
                        stack.pop();

                        if (level > 0)
                        {
                                s += '\n';
                                s += std::string((level - 1) * TEXT_INDENT, ' ');
                                s += m_nodes[index].name;
                        }

                        for (auto iter = m_nodes[index].children.crbegin(); iter != m_nodes[index].children.crend();
                             ++iter)
                        {
                                stack.push({*iter, level + 1});
                        }
                }

                return s;
        }
};

void device_type(const PhysicalDevice& device, size_t device_node, Tree* tree)
{
        size_t type_node = tree->add(device_node, "Device Type");
        tree->add(type_node, physical_device_type_to_string(device.properties().deviceType));
}

void api_version(const PhysicalDevice& device, size_t device_node, Tree* tree)
{
        size_t api_node = tree->add(device_node, "API Version");
        tree->add(api_node, api_version_to_string(device.properties().apiVersion));
}

void extensions(const PhysicalDevice& device, size_t device_node, Tree* tree)
{
        size_t extensions_node = tree->add(device_node, "Extensions");

        try
        {
                for (const std::string& e : sorted(device.supported_extensions()))
                {
                        tree->add(extensions_node, e);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(extensions_node, e.what());
        }
}

template <class T>
std::string as_string(const T& v)
{
        return to_string(v);
}

template <class T, std::size_t N>
std::string as_string(T (&array)[N])
{
        static_assert(N > 0);
        std::string s;
        s += "(";
        s += to_string(array[0]);
        for (size_t i = 1; i < N; ++i)
        {
                s += ", ";
                s += to_string(array[i]);
        }
        s += ")";
        return s;
}

#define ADD_LIMIT(v) add(#v, device.properties().limits.v)

void limits(const PhysicalDevice& device, size_t device_node, Tree* tree)
{
        size_t limits_node = tree->add(device_node, "Limits");

        try
        {
                auto add = [&](const std::string& s, const auto& v) {
                        tree->add(limits_node, s + " = " + as_string(v));
                };

                ADD_LIMIT(maxImageDimension1D);
                ADD_LIMIT(maxImageDimension2D);
                ADD_LIMIT(maxImageDimension3D);
                ADD_LIMIT(maxPushConstantsSize);
                ADD_LIMIT(maxComputeSharedMemorySize);
                ADD_LIMIT(maxComputeWorkGroupCount);
                ADD_LIMIT(maxComputeWorkGroupInvocations);
                ADD_LIMIT(maxComputeWorkGroupSize);
                ADD_LIMIT(maxClipDistances);
        }
        catch (const std::exception& e)
        {
                tree->add(limits_node, e.what());
        }
}

void queues(
        const PhysicalDevice& device,
        const std::vector<VkQueueFamilyProperties>& families,
        size_t family_index,
        size_t queue_families_node,
        Tree* tree)
{
        const VkQueueFamilyProperties& p = families[family_index];

        size_t queue_family_node = tree->add(queue_families_node, "Family " + to_string(family_index));

        try
        {
                tree->add(queue_family_node, "queue count: " + to_string(p.queueCount));

                if (p.queueCount < 1)
                {
                        return;
                }

                if (p.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                        tree->add(queue_family_node, "graphics");
                }
                if (p.queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                        tree->add(queue_family_node, "compute");
                }
                if (p.queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                        tree->add(queue_family_node, "transfer");
                }
                if (p.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
                {
                        tree->add(queue_family_node, "sparse binding");
                }
                if (p.queueFlags & VK_QUEUE_PROTECTED_BIT)
                {
                        tree->add(queue_family_node, "protected");
                }

                if (device.queue_family_supports_presentation(family_index))
                {
                        tree->add(queue_family_node, "presentation");
                }
        }
        catch (const std::exception& e)
        {
                tree->add(queue_family_node, e.what());
        }
}

void queue_families(const PhysicalDevice& device, size_t device_node, Tree* tree)
{
        size_t queue_families_node = tree->add(device_node, "QueueFamilies");

        try
        {
                std::vector<VkQueueFamilyProperties> families = device.queue_families();

                for (size_t family_index = 0; family_index < families.size(); ++family_index)
                {
                        queues(device, families, family_index, queue_families_node, tree);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(queue_families_node, e.what());
        }
}
}

std::string overview()
{
        Tree tree;

        size_t api_node = tree.add("API Version");
        try
        {
                tree.add(api_node, api_version_to_string(supported_instance_api_version()));
        }
        catch (const std::exception& e)
        {
                tree.add(api_node, e.what());
        }

        size_t extensions_node = tree.add("Extensions");
        try
        {
                for (const std::string& extension : sorted(supported_instance_extensions()))
                {
                        tree.add(extensions_node, extension);
                }
        }
        catch (const std::exception& e)
        {
                tree.add(extensions_node, e.what());
        }

        size_t validation_layers_node = tree.add("Validation Layers");
        try
        {
                for (const std::string& layer : sorted(supported_validation_layers()))
                {
                        tree.add(validation_layers_node, layer);
                }
        }
        catch (const std::exception& e)
        {
                tree.add(validation_layers_node, e.what());
        }

        size_t required_surface_extensions_node = tree.add("Required Surface Extensions");
        try
        {
                for (const std::string& extension : sorted(vulkan_create_surface_extensions()))
                {
                        tree.add(required_surface_extensions_node, extension);
                }
        }
        catch (const std::exception& e)
        {
                tree.add(required_surface_extensions_node, e.what());
        }

        return tree.text();
}

std::string overview_physical_devices(VkInstance instance, VkSurfaceKHR surface)
{
        Tree tree;

        size_t physical_devices_node = tree.add("Physical Devices");

        for (const VkPhysicalDevice& d : physical_devices(instance))
        {
                PhysicalDevice device(d, surface);

                size_t device_node = tree.add(physical_devices_node, device.properties().deviceName);

                device_type(device, device_node, &tree);
                api_version(device, device_node, &tree);
                extensions(device, device_node, &tree);
                limits(device, device_node, &tree);
                queue_families(device, device_node, &tree);
        }

        return tree.text();
}
}
