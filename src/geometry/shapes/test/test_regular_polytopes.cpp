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

#include "../regular_polytopes.h"

#include <src/com/error.h>
#include <src/test/test.h>

namespace ns::geometry
{
namespace
{
template <std::size_t N, typename T>
void test_simplex(const std::array<Vector<N, T>, N + 1>& vertices)
{
        static_assert(N >= 2);

        for (const Vector<N, T>& v : vertices)
        {
                if (std::abs(1 - v.norm()) > T(1e-3))
                {
                        error("Simplex is not origin-centered");
                }
        }

        const T distance = (vertices[0] - vertices[1]).norm();
        for (unsigned i = 0; i < vertices.size(); ++i)
        {
                for (unsigned j = i + 1; j < vertices.size(); ++j)
                {
                        if (std::abs(distance - (vertices[i] - vertices[j]).norm()) > T(1e-3))
                        {
                                error("Simplex vertices are not equidistant");
                        }
                }
        }
}

template <std::size_t N, typename T>
void test_simplex()
{
        std::array<Vector<N, T>, N + 1> simplex = create_origin_centered_simplex<N, T>();
        test_simplex(simplex);
}

template <std::size_t N>
void test_simplex()
{
        test_simplex<N, float>();
        test_simplex<N, double>();
        test_simplex<N, long double>();
}

void test()
{
        test_simplex<2>();
        test_simplex<3>();
        test_simplex<4>();
        test_simplex<5>();
}

TEST_SMALL("Test simplex", test)
}
}
