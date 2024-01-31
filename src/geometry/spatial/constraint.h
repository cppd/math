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

#include <src/numerical/vector.h>

#include <array>
#include <cstddef>

namespace ns::geometry::spatial
{
// a * x + b
template <std::size_t N, typename T>
struct Constraint final
{
        numerical::Vector<N, T> a;
        T b;
};

template <std::size_t N, typename T, std::size_t COUNT, std::size_t COUNT_EQ>
struct Constraints final
{
        std::array<Constraint<N, T>, COUNT> c;
        std::array<Constraint<N, T>, COUNT_EQ> c_eq;
};

template <std::size_t N, typename T, std::size_t COUNT>
struct Constraints<N, T, COUNT, 0> final
{
        std::array<Constraint<N, T>, COUNT> c;
};
}
