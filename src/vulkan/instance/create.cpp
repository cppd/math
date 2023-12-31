/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "create.h"

#include "info.h"

#include "../api_version.h"
#include "../objects.h"
#include "../overview.h"
#include "../strings.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/string/strings.h>
#include <src/settings/name.h>

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace ns::vulkan
{
namespace
{
void check_api_version()
{
        const std::uint32_t api_version = supported_instance_api_version();

        if (api_version_suitable(api_version))
        {
                return;
        }

        std::ostringstream oss;
        oss << "Vulkan instance API version ";
        oss << api_version_to_string(api_version);
        oss << " is not supported, minimum required version is ";
        oss << api_version_to_string(API_VERSION);
        error(oss.str());
}

void check_layer_support(const std::unordered_set<std::string>& required_layers)
{
        if (required_layers.empty())
        {
                return;
        }

        const std::unordered_set<std::string> supported = supported_instance_layers();

        for (const std::string& layer : required_layers)
        {
                if (!supported.contains(layer))
                {
                        error("Vulkan layer " + layer + " is not supported");
                }
        }
}

void check_extension_support(const std::unordered_set<std::string>& required_extensions)
{
        if (required_extensions.empty())
        {
                return;
        }

        const std::unordered_set<std::string> supported = supported_instance_extensions();

        for (const std::string& extension : required_extensions)
        {
                if (!supported.contains(extension))
                {
                        error("Vulkan instance extension " + extension + " is not supported");
                }
        }
}

std::string instance_info(const std::vector<const char*>& extensions, const std::vector<const char*>& layers)
{
        std::string s;
        s = "Vulkan instance extensions: {" + strings_to_sorted_string(extensions, ", ") + "}";
        s += '\n';
        s += "Vulkan instance layers: {" + strings_to_sorted_string(layers, ", ") + "}";
        return s;
}
}

handle::Instance create_instance(
        const std::unordered_set<std::string>& required_layers,
        const std::unordered_set<std::string>& required_extensions)
{
        LOG(overview());

        check_api_version();
        check_layer_support(required_layers);
        check_extension_support(required_extensions);

        const std::vector<const char*> extensions = strings_to_char_pointers(required_extensions);
        const std::vector<const char*> layers = strings_to_char_pointers(required_layers);

        LOG(instance_info(extensions, layers));

        VkApplicationInfo app_info = {};
        app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName = settings::APPLICATION_NAME;
        app_info.applicationVersion = 1;
        app_info.pEngineName = nullptr;
        app_info.engineVersion = 0;
        app_info.apiVersion = API_VERSION;

        VkInstanceCreateInfo create_info = {};
        create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = extensions.size();
        create_info.ppEnabledExtensionNames = extensions.data();
        create_info.enabledLayerCount = layers.size();
        create_info.ppEnabledLayerNames = layers.data();

        return handle::Instance(create_info);
}
}
