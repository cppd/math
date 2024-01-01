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

#pragma once

#include <algorithm>
#include <type_traits>

namespace ns::gpu::renderer
{
template <typename T>
[[nodiscard]] T clean_ambient(const T ambient)
{
        static_assert(std::is_floating_point_v<T>);
        return std::clamp<T>(ambient, 0, 1);
}

template <typename T>
[[nodiscard]] T clean_metalness(const T metalness)
{
        static_assert(std::is_floating_point_v<T>);
        return std::clamp<T>(metalness, 0, 1);
}

template <typename T>
[[nodiscard]] T clean_roughness(const T roughness)
{
        static_assert(std::is_floating_point_v<T>);
        return std::clamp<T>(roughness, 0, 1);
}
}
