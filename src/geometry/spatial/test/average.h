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

#include <type_traits>

namespace ns::geometry::spatial::test
{
template <int COUNT, typename F>
double average(const F& f)
{
        static_assert(std::is_same_v<double, decltype(f())>);

        double sum = 0;
        for (int i = 0; i < COUNT; ++i)
        {
                sum += f();
        }
        return sum / COUNT;
}
}