/*
Copyright (C) 2017-2023 Topological Manifold

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

namespace ns::geometry::spatial::intersection
{
template <int COUNT, typename F>
auto average(const F& f)
{
        using T = std::remove_cvref_t<decltype(f())>;
        static_assert(std::is_floating_point_v<T>);
        static_assert(COUNT > 0);

        T sum = 0;
        for (int i = 0; i < COUNT; ++i)
        {
                sum += f();
        }
        return sum / COUNT;
}
}
