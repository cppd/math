/*
Copyright (C) 2017-2022 Topological Manifold

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

#include "instance_info.h"

#include "error.h"

#include <vector>

namespace ns::vulkan
{
std::unordered_set<std::string> supported_instance_extensions()
{
        std::uint32_t extension_count;
        VULKAN_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr));

        if (extension_count < 1)
        {
                return {};
        }

        std::vector<VkExtensionProperties> extensions(extension_count);
        VULKAN_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data()));

        std::unordered_set<std::string> extension_set;
        for (const VkExtensionProperties& e : extensions)
        {
                extension_set.emplace(e.extensionName);
        }
        return extension_set;
}

std::unordered_set<std::string> supported_instance_layers()
{
        std::uint32_t layer_count;
        VULKAN_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));

        if (layer_count < 1)
        {
                return {};
        }

        std::vector<VkLayerProperties> layers(layer_count);
        VULKAN_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, layers.data()));

        std::unordered_set<std::string> layer_set;
        for (const VkLayerProperties& l : layers)
        {
                layer_set.emplace(l.layerName);
        }
        return layer_set;
}

std::uint32_t supported_instance_api_version()
{
        std::uint32_t api_version;
        VULKAN_CHECK(vkEnumerateInstanceVersion(&api_version));
        return api_version;
}
}
