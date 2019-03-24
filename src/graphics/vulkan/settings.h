/*
Copyright (C) 2017-2019 Topological Manifold

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

#include <string>
#include <vector>

namespace vulkan
{
constexpr int API_VERSION_MAJOR = 1;
constexpr int API_VERSION_MINOR = 1;

// clang-format off
constexpr std::initializer_list<const char*> VALIDATION_LAYERS
{
        "VK_LAYER_LUNARG_standard_validation"
};
// clang-format on
}
