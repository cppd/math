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

/*
 Sasho Kalajdzievski.
 An Illustrated Introduction to Topology and Homotopy.
 CRC Press, 2015.

 5.1 Finite Products of Spaces.
 14.4 Regarding Classification of CW-Complexes and Higher Dimensional Manifolds.
*/

#pragma once

#include <src/sampling/sphere_uniform.h>

namespace ns::geometry
{
template <std::size_t N, typename T, typename RandomEngine>
Vector<N, T> torus_point(RandomEngine& random_engine)
{
        static_assert(N >= 3);

        Vector<N, T> v(0);
        T v_length;
        Vector<N, T> sum(0);

        v[0] = 2;
        v_length = 2;

        for (std::size_t n = 1; n < N; ++n)
        {
                Vector<N, T> ortho(0);
                ortho[n] = v_length;

                Vector<2, T> s = sampling::uniform_on_sphere<2, T>(random_engine);
                Vector<N, T> vn = T(0.5) * (s[0] * v + s[1] * ortho);
                sum += vn;

                v = vn;
                v_length *= 0.5;
        }
        return sum;
}
}
