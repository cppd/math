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

#include <src/numerical/vector.h>

#include <array>

namespace ns::filter
{
struct Mean final
{
        template <std::size_t N, typename T, std::size_t COUNT>
        [[nodiscard]] Vector<N, T> operator()(const std::array<Vector<N, T>, COUNT>& p, const Vector<COUNT, T>& w) const
        {
                static_assert(COUNT > 0);
                Vector<N, T> x = p[0] * w[0];
                for (std::size_t i = 1; i < COUNT; ++i)
                {
                        x.multiply_add(p[i], w[i]);
                }
                return x;
        }
};

struct Add final
{
        template <std::size_t N, typename T>
        [[nodiscard]] Vector<N, T> operator()(const Vector<N, T>& a, const Vector<N, T>& b) const
        {
                return a + b;
        }
};

struct Subtract final
{
        template <std::size_t N, typename T>
        [[nodiscard]] Vector<N, T> operator()(const Vector<N, T>& a, const Vector<N, T>& b) const
        {
                return a - b;
        }
};
}
