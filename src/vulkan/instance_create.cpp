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

#include "instance_create.h"

#include "instance_info.h"
#include "overview.h"
#include "print.h"
#include "settings.h"

#include <src/com/alg.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/string/vector.h>
#include <src/settings/name.h>

#include <unordered_set>

namespace ns::vulkan
{
namespace
{
void check_extension_support(const std::vector<std::string>& required_extensions)
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

void check_layer_support(const std::vector<std::string>& required_layers)
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

void check_api_version()
{
        const std::uint32_t api_version = supported_instance_api_version();

        if (!api_version_suitable(api_version))
        {
                error("Vulkan instance API version " + api_version_to_string(API_VERSION)
                      + " is not supported. Supported " + api_version_to_string(api_version) + ".");
        }
}
}

Instance create_instance(std::vector<std::string> required_extensions)
{
        LOG(overview());

        check_api_version();

        if (!LAYERS.empty())
        {
                required_extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        sort_and_unique(&required_extensions);

        check_extension_support(required_extensions);

        if (!LAYERS.empty())
        {
                check_layer_support({LAYERS.cbegin(), LAYERS.cend()});
        }

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

        std::string info;

        create_info.enabledExtensionCount = extensions.size();
        create_info.ppEnabledExtensionNames = extensions.data();
        info = "Vulkan instance extensions: {" + strings_to_sorted_string(extensions) + "}";

        create_info.enabledLayerCount = LAYERS.size();
        create_info.ppEnabledLayerNames = LAYERS.data();
        info += "\nVulkan instance layers: {" + strings_to_sorted_string(LAYERS) + "}";

        LOG(info);

        return Instance(create_info);
}
}
