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

#pragma once

#include <vulkan/vulkan_core.h>

#include <cstdint>

namespace ns::vulkan
{
inline constexpr std::uint32_t API_VERSION_VARIANT = 0;
inline constexpr std::uint32_t API_VERSION_MAJOR = (static_cast<unsigned>(BUILD_VULKAN_API_VERSION) >> 8) & 0xFFu;
inline constexpr std::uint32_t API_VERSION_MINOR = static_cast<unsigned>(BUILD_VULKAN_API_VERSION) & 0xFFu;

inline constexpr std::uint32_t API_VERSION =
        VK_MAKE_API_VERSION(API_VERSION_VARIANT, API_VERSION_MAJOR, API_VERSION_MINOR, 0);

inline bool api_version_suitable(const std::uint32_t api_version)
{
        if (VK_API_VERSION_VARIANT(api_version) != API_VERSION_VARIANT)
        {
                return false;
        }

        if (VK_API_VERSION_MAJOR(api_version) > API_VERSION_MAJOR)
        {
                return true;
        }

        if (VK_API_VERSION_MAJOR(api_version) < API_VERSION_MAJOR)
        {
                return false;
        }

        return VK_API_VERSION_MINOR(api_version) >= API_VERSION_MINOR;
}
}
