/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <array>
#include <vulkan/vulkan.h>

namespace ns::vulkan
{
inline constexpr int API_VERSION_MAJOR = 1;
inline constexpr int API_VERSION_MINOR = 2;
inline constexpr uint32_t API_VERSION = VK_MAKE_VERSION(API_VERSION_MAJOR, API_VERSION_MINOR, 0);

inline constexpr std::array VALIDATION_LAYERS = std::to_array<const char*>({"VK_LAYER_KHRONOS_validation"});
}
