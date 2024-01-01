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

#include "../sphere_create.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/type/name.h>
#include <src/geometry/core/check.h>
#include <src/geometry/core/euler.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace ns::geometry::shapes::test
{
namespace
{
template <std::size_t N, typename T>
void test_sphere_creation()
{
        const std::string name = to_string(N - 1) + "-sphere";

        LOG("Test " + name + " creation in " + space_name(N) + ", " + type_name<T>());

        constexpr unsigned FACET_COUNT = 1000;

        std::vector<Vector<N, T>> vertices;
        std::vector<std::array<int, N>> facets;

        create_sphere(FACET_COUNT, &vertices, &facets);
        LOG(name + ": vertex count = " + to_string(vertices.size()) + ", facet count = " + to_string(facets.size()));

        if (facets.size() < FACET_COUNT)
        {
                error(name + " facet count " + to_string(facets.size()) + " is less than required minimum "
                      + to_string(FACET_COUNT));
        }

        constexpr bool HAS_BOUNDARY = false;
        core::check_mesh(name, vertices, facets, HAS_BOUNDARY, core::euler_characteristic_for_convex_polytope<N>());
}

template <std::size_t N>
void test_sphere_creation()
{
        test_sphere_creation<N, float>();
        test_sphere_creation<N, double>();
}

void test()
{
        test_sphere_creation<3>();
        test_sphere_creation<4>();
        test_sphere_creation<5>();
}

TEST_SMALL("Sphere Creation", test)
}
}
