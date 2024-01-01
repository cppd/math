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

#include <bit>
#include <type_traits>

namespace ns
{
template <typename T>
constexpr T bit_width(T value)
{
        static_assert(std::is_unsigned_v<T> || std::is_same_v<std::remove_cv_t<T>, unsigned __int128>);

        constexpr bool HAS_BIT_WIDTH = requires { std::bit_width(value); };

        if constexpr (HAS_BIT_WIDTH)
        {
                return std::bit_width(value);
        }
        else
        {
                if (value == 0)
                {
                        return 0;
                }

                T res = 1;
                while (value >>= 1)
                {
                        ++res;
                }
                return res;
        }
}
}
