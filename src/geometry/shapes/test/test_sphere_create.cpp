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

#include "../sphere_create.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/sort.h>
#include <src/com/type/name.h>
#include <src/geometry/core/euler.h>
#include <src/numerical/orthogonal.h>
#include <src/test/test.h>

#include <sstream>
#include <unordered_map>

namespace ns::geometry
{
namespace
{
template <std::size_t N>
void check_euler_characteristic(const std::vector<std::array<int, N>>& facets)
{
        constexpr int EXPECTED_EULER_CHARACTERISTIC = euler_characteristic_for_convex_polytope<N>();

        const int computed_euler_characteristic = euler_characteristic(facets);

        if (computed_euler_characteristic == EXPECTED_EULER_CHARACTERISTIC)
        {
                return;
        }

        std::ostringstream oss;

        oss << (N - 1) << "-sphere Euler characteristic (" << computed_euler_characteristic << ")";
        oss << " is not equal to " << EXPECTED_EULER_CHARACTERISTIC;

        std::array<long long, N> counts = simplex_counts(facets);
        for (unsigned i = 0; i < N; ++i)
        {
                oss << '\n' << i << "-simplex count = " << counts[i];
        }

        error(oss.str());
}

template <std::size_t N, typename T>
void check_manifold(const std::vector<Vector<N, T>>& vertices, const std::vector<std::array<int, N>>& facets)
{
        struct Hash
        {
                std::size_t operator()(const std::array<int, N - 1>& v) const
                {
                        return array_hash(v);
                }
        };

        std::unordered_map<std::array<int, N - 1>, int, Hash> ridges;

        for (const std::array<int, N>& facet : facets)
        {
                Vector<N, T> n = numerical::ortho_nn(vertices, facet).normalized();
                if (!is_finite(n))
                {
                        error("Facet normal " + to_string(n) + " is not finite");
                }

                for (unsigned r = 0; r < N; ++r)
                {
                        std::array<int, N - 1> ridge = sort(del_elem(facet, r));
                        ++ridges[ridge];
                }
        }

        for (const auto& [ridge, count] : ridges)
        {
                if (count != 2)
                {
                        error("Facet count " + to_string(count) + " is not equal to 2 for ridge " + to_string(ridge));
                }
        }
}

template <std::size_t N, typename T>
void test_sphere_creation()
{
        LOG("Test sphere creation in " + space_name(N) + ", " + type_name<T>());

        constexpr unsigned FACET_COUNT = 1000;

        std::vector<Vector<N, T>> vertices;
        std::vector<std::array<int, N>> facets;

        create_sphere(FACET_COUNT, &vertices, &facets);

        if (facets.size() < FACET_COUNT)
        {
                error("Facet count " + to_string(facets.size()) + " is less than required minimum "
                      + to_string(FACET_COUNT));
        }

        check_manifold(vertices, facets);
        check_euler_characteristic(facets);
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
