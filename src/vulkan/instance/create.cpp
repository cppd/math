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

#include "create.h"

#include "info.h"

#include "../api_version.h"
#include "../overview.h"
#include "../print.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/string/vector.h>
#include <src/settings/name.h>

#include <sstream>

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
}

handle::Instance create_instance(
        const std::unordered_set<std::string>& required_layers,
        const std::unordered_set<std::string>& required_extensions)
{
        LOG(overview());

        check_api_version();
        check_layer_support(required_layers);
        check_extension_support(required_extensions);

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

        const std::vector<const char*> extensions = const_char_pointer_vector(&required_extensions);
        const std::vector<const char*> layers = const_char_pointer_vector(&required_layers);

        std::string info;

        create_info.enabledExtensionCount = extensions.size();
        create_info.ppEnabledExtensionNames = extensions.data();
        info = "Vulkan instance extensions: {" + strings_to_sorted_string(extensions, ", ") + "}";

        create_info.enabledLayerCount = layers.size();
        create_info.ppEnabledLayerNames = layers.data();
        info += "\nVulkan instance layers: {" + strings_to_sorted_string(layers, ", ") + "}";

        LOG(info);

        return handle::Instance(create_info);
}
}
