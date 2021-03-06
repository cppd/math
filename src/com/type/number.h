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

#include "limit.h"
#include "trait.h"

namespace ns
{
namespace number_implementation
{
template <typename T>
constexpr T previous_before_one()
{
        static_assert(is_native_floating_point<T>);
        static_assert(limits<T>::radix == 2);

        T prev_e = 1;
        T e = 1;
        do
        {
                prev_e = e;
                e /= 2;
        } while (1 - e != 1);
        return 1 - prev_e;
}
}

template <typename T>
inline constexpr T PREVIOUS_BEFORE_ONE = number_implementation::previous_before_one<T>();
}
