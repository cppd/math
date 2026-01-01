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

#include <type_traits>

namespace ns
{
template <typename T>
        requires std::is_enum_v<T>
[[nodiscard]] constexpr auto enum_to_int(const T v)
{
        if constexpr (std::is_signed_v<std::underlying_type_t<T>>)
        {
                return static_cast<std::common_type_t<int, std::underlying_type_t<T>>>(v);
        }
        else
        {
                return static_cast<std::common_type_t<unsigned, std::underlying_type_t<T>>>(v);
        }
}
}
