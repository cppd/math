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

#include "instance.h"

#include "instance/create.h"
#include "instance/debug.h"
#include "instance/info.h"

#include <src/com/error.h>
#include <src/vulkan/extensions.h>
#include <src/vulkan/objects.h>
#include <src/window/surface.h>

#include <vulkan/vulkan_core.h>

#include <atomic>
#include <memory>
#include <string>
#include <unordered_set>

namespace ns::vulkan
{
namespace
{
std::unordered_set<std::string> layers()
{
        std::unordered_set<std::string> layers;

#ifndef BUILD_RELEASE
        layers.insert("VK_LAYER_KHRONOS_validation");
#endif

        if (!layers.empty())
        {
                const std::unordered_set<std::string> supported = instance::supported_layers();
                for (const std::string& s : layers)
                {
                        if (!supported.contains(s))
                        {
                                error("Vulkan layer " + s + " is not supported");
                        }
                }
        }

        return layers;
}

std::unordered_set<std::string> extensions()
{
        std::unordered_set<std::string> extensions;

        extensions.insert(window::vulkan_create_surface_extension());
        extensions.insert(VK_KHR_SURFACE_EXTENSION_NAME);
        extensions.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        if (!extensions.empty())
        {
                const std::unordered_set<std::string> supported = instance::supported_extensions();
                for (const std::string& s : extensions)
                {
                        if (!supported.contains(s))
                        {
                                error("Vulkan instance extension " + s + " is not supported");
                        }
                }
        }

        return extensions;
}
}

class Instance::Impl final
{
        handle::Instance instance_;
        InstanceExtensionFunctions instance_extension_functions_;
        handle::DebugUtilsMessengerEXT messenger_;

        Impl(const std::unordered_set<std::string>& layers, const std::unordered_set<std::string>& extensions)
                : instance_(instance::create_instance(layers, extensions)),
                  instance_extension_functions_(instance_)
        {
                if (extensions.contains(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
                {
                        messenger_ = instance::create_debug_utils_messenger(instance_);
                }
        }

public:
        Impl()
                : Impl(layers(), extensions())
        {
        }

        [[nodiscard]] VkInstance handle() const
        {
                return instance_;
        }
};

Instance::Instance()
{
        static std::atomic_int call_counter = 0;
        if (++call_counter != 1)
        {
                error_fatal("Vulkan instance must be created once");
        }

        instance_ = std::make_unique<Impl>();

        instance_handle_ = instance_->handle();
}

Instance::~Instance()
{
        instance_handle_ = VK_NULL_HANDLE;
}
}
