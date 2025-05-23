/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "strings.h"

#include "instance/info.h"
#include "physical_device/features.h"
#include "physical_device/find.h"
#include "physical_device/physical_device.h"
#include "physical_device/properties.h"

#include <src/com/print.h>
#include <src/com/string_tree.h>
#include <src/com/variant.h>
#include <src/window/surface.h>

#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace ns::vulkan
{
namespace
{
constexpr unsigned TREE_LEVEL_INDENT = 2;

template <typename T>
std::vector<std::string> sorted(const T& s)
{
        std::vector<std::string> res(s.cbegin(), s.cend());
        std::ranges::sort(res);
        return res;
}

void add_strings(
        std::string name,
        const std::variant<std::string, std::vector<std::string>>& strings,
        const std::size_t node,
        StringTree* const tree)
{
        const auto string = [&](const std::string& s)
        {
                name += " = ";
                name += s;
                tree->add(node, std::move(name));
        };

        const auto vector = [&](const std::vector<std::string>& v)
        {
                if (v.empty())
                {
                        name += " = ";
                        tree->add(node, std::move(name));
                        return;
                }

                const std::size_t n = tree->add(node, std::move(name));
                for (const std::string& s : v)
                {
                        tree->add(n, s);
                }
        };

        std::visit(Visitors{string, vector}, strings);
}

void conformance_version(
        const physical_device::PhysicalDevice& device,
        const std::size_t device_node,
        StringTree* const tree)
{
        const std::size_t node = tree->add(device_node, "Conformance Version");

        const VkConformanceVersion version = device.properties().properties_12.conformanceVersion;
        std::ostringstream oss;
        oss << static_cast<int>(version.major);
        oss << "." << static_cast<int>(version.minor);
        oss << "." << static_cast<int>(version.subminor);
        oss << "." << static_cast<int>(version.patch);
        tree->add(node, oss.str());
}

void device_name(const physical_device::PhysicalDevice& device, const std::size_t device_node, StringTree* const tree)
{
        const std::size_t node = tree->add(device_node, "Device Name");
        const std::string device_name = static_cast<const char*>(device.properties().properties_10.deviceName);
        tree->add(node, device_name);
}

void driver_info(const physical_device::PhysicalDevice& device, const std::size_t device_node, StringTree* const tree)
{
        const std::size_t node = tree->add(device_node, "Driver");
        const std::string driver_name = static_cast<const char*>(device.properties().properties_12.driverName);
        const std::string driver_info = static_cast<const char*>(device.properties().properties_12.driverInfo);
        tree->add(node, "Name = " + driver_name);
        tree->add(node, "Info = " + driver_info);
}

void device_type(const physical_device::PhysicalDevice& device, const std::size_t device_node, StringTree* const tree)
{
        const std::size_t node = tree->add(device_node, "Device Type");
        try
        {
                tree->add(node, strings::physical_device_type_to_string(device.properties().properties_10.deviceType));
        }
        catch (const std::exception& e)
        {
                tree->add(node, e.what());
        }
}

void api_version(const physical_device::PhysicalDevice& device, const std::size_t device_node, StringTree* const tree)
{
        const std::size_t node = tree->add(device_node, "API Version");
        try
        {
                tree->add(node, strings::api_version_to_string(device.properties().properties_10.apiVersion));
        }
        catch (const std::exception& e)
        {
                tree->add(node, e.what());
        }
}

void extensions(const physical_device::PhysicalDevice& device, const std::size_t device_node, StringTree* const tree)
{
        const std::size_t node = tree->add(device_node, "Extensions");
        try
        {
                for (const std::string& e : sorted(device.extensions()))
                {
                        tree->add(node, e);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(node, e.what());
        }
}

void properties(const physical_device::PhysicalDevice& device, const std::size_t device_node, StringTree* const tree)
{
        const std::size_t node = tree->add(device_node, "Properties");
        try
        {
                for (auto& [name, strings] : device_properties_to_strings(device.properties()))
                {
                        add_strings(name, strings, node, tree);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(node, e.what());
        }
}

void features(const physical_device::PhysicalDevice& device, const std::size_t device_node, StringTree* const tree)
{
        const std::size_t features_node = tree->add(device_node, "Features");
        const std::size_t supported_node = tree->add(features_node, "Supported");
        const std::size_t not_supported_node = tree->add(features_node, "Not Supported");
        try
        {
                for (const std::string& name : sorted(features_to_strings(device.features(), true)))
                {
                        tree->add(supported_node, name);
                }

                for (const std::string& name : sorted(features_to_strings(device.features(), false)))
                {
                        tree->add(not_supported_node, name);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(features_node, e.what());
        }
}

void family_queues(
        const physical_device::PhysicalDevice& device,
        const VkQueueFamilyProperties& family_properties,
        const std::size_t family_index,
        const std::size_t queue_families_node,
        StringTree* const tree)
{
        const std::size_t node = tree->add(queue_families_node, "Family " + to_string(family_index));
        try
        {
                tree->add(node, "queue count: " + to_string(family_properties.queueCount));

                if (family_properties.queueCount < 1)
                {
                        return;
                }

                add_strings("queueFlags", strings::queues_to_strings(family_properties.queueFlags), node, tree);

                if (device.queue_family_supports_presentation(family_index))
                {
                        tree->add(node, "presentation");
                }
        }
        catch (const std::exception& e)
        {
                tree->add(node, e.what());
        }
}

void queue_families(
        const physical_device::PhysicalDevice& device,
        const std::size_t device_node,
        StringTree* const tree)
{
        const std::size_t node = tree->add(device_node, "QueueFamilies");
        try
        {
                for (std::size_t index = 0; const VkQueueFamilyProperties& properties : device.queue_families())
                {
                        family_queues(device, properties, index++, node, tree);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(node, e.what());
        }
}

void api_version(StringTree* const tree)
{
        const std::size_t node = tree->add("API Version");
        try
        {
                tree->add(node, strings::api_version_to_string(instance::supported_api_version()));
        }
        catch (const std::exception& e)
        {
                tree->add(node, e.what());
        }
}

void extensions(StringTree* const tree)
{
        const std::size_t node = tree->add("Extensions");
        try
        {
                for (const std::string& extension : sorted(instance::supported_extensions()))
                {
                        tree->add(node, extension);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(node, e.what());
        }
}

void layers(StringTree* const tree)
{
        const std::size_t node = tree->add("Layers");
        try
        {
                for (const std::string& layer : sorted(instance::supported_layers()))
                {
                        tree->add(node, layer);
                }
        }
        catch (const std::exception& e)
        {
                tree->add(node, e.what());
        }
}

void create_surface_extension(StringTree* const tree)
{
        const std::size_t node = tree->add("Create Surface Extension");
        try
        {
                tree->add(node, window::vulkan_create_surface_extension());
        }
        catch (const std::exception& e)
        {
                tree->add(node, e.what());
        }
}
}

std::string overview()
{
        StringTree tree;

        api_version(&tree);
        extensions(&tree);
        layers(&tree);
        create_surface_extension(&tree);

        return tree.text(TREE_LEVEL_INDENT);
}

std::string overview_physical_devices(const VkInstance instance, const VkSurfaceKHR surface)
{
        StringTree tree;

        std::unordered_set<std::string> uuids;

        for (const VkPhysicalDevice device : physical_device::find_devices(instance))
        {
                const physical_device::PhysicalDevice physical_device(device, surface);

                if (!uuids.emplace(to_string(physical_device.properties().properties_10.pipelineCacheUUID)).second)
                {
                        continue;
                }

                const std::size_t node = tree.add("Physical Device");

                device_name(physical_device, node, &tree);
                device_type(physical_device, node, &tree);
                api_version(physical_device, node, &tree);
                driver_info(physical_device, node, &tree);
                conformance_version(physical_device, node, &tree);
                extensions(physical_device, node, &tree);
                properties(physical_device, node, &tree);
                features(physical_device, node, &tree);
                queue_families(physical_device, node, &tree);
        }

        return tree.text(TREE_LEVEL_INDENT);
}
}
