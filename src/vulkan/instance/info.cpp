/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "info.h"

#include <src/vulkan/error.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace ns::vulkan::instance
{
std::unordered_set<std::string> supported_extensions()
{
        std::uint32_t count;
        VULKAN_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr));

        std::vector<VkExtensionProperties> extensions(count);

        if (count > 0)
        {
                VULKAN_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()));
        }

        std::unordered_set<std::string> res;
        res.reserve(extensions.size());
        for (const VkExtensionProperties& extension : extensions)
        {
                res.emplace(extension.extensionName);
        }
        return res;
}

std::unordered_set<std::string> supported_layers()
{
        std::uint32_t count;
        VULKAN_CHECK(vkEnumerateInstanceLayerProperties(&count, nullptr));

        std::vector<VkLayerProperties> layers(count);

        if (count > 0)
        {
                VULKAN_CHECK(vkEnumerateInstanceLayerProperties(&count, layers.data()));
        }

        std::unordered_set<std::string> res;
        res.reserve(layers.size());
        for (const VkLayerProperties& layer : layers)
        {
                res.emplace(layer.layerName);
        }
        return res;
}

std::uint32_t supported_api_version()
{
        std::uint32_t api_version;
        VULKAN_CHECK(vkEnumerateInstanceVersion(&api_version));
        return api_version;
}
}
