/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "api_version.h"

#include <vulkan/vulkan_core.h>

#include <cstdint>
#include <string>

namespace ns::vulkan
{
std::string api_version_to_string(const std::uint32_t api_version)
{
        std::string s;
        s += std::to_string(VK_API_VERSION_VARIANT(api_version));
        s += ".";
        s += std::to_string(VK_API_VERSION_MAJOR(api_version));
        s += ".";
        s += std::to_string(VK_API_VERSION_MINOR(api_version));
        s += ".";
        s += std::to_string(VK_API_VERSION_PATCH(api_version));
        return s;
}
}
