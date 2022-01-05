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

#pragma once

#include <utility>
#include <vulkan/vulkan.h>

namespace ns::vulkan
{
PFN_vkVoidFunction proc_addr(VkInstance instance, const char* name);
}

#define VULKAN_EXTENSION_FUNCTION(name)                                 \
        template <typename... T>                                        \
        inline decltype(auto) name(const VkInstance instance, T&&... v) \
        {                                                               \
                const auto p = ns::vulkan::proc_addr(instance, #name);  \
                const auto f = reinterpret_cast<PFN_##name>(p);         \
                return f(instance, std::forward<T>(v)...);              \
        }

VULKAN_EXTENSION_FUNCTION(vkCreateDebugReportCallbackEXT)
VULKAN_EXTENSION_FUNCTION(vkDestroyDebugReportCallbackEXT)
