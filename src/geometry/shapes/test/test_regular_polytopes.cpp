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

#include "../mesh.h"
#include "../regular_polytopes.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/geometry/core/euler.h>
#include <src/test/test.h>

#include <sstream>
#include <unordered_set>

namespace ns::geometry
{
namespace
{
template <typename T>
constexpr T ABS_ERROR = 10 * limits<T>::epsilon();

template <std::size_t N, typename T>
void test_simplex(const std::array<Vector<N, T>, N + 1>& vertices)
{
        static_assert(N >= 2);

        for (const Vector<N, T>& v : vertices)
        {
                if (!(std::abs(1 - v.norm()) < ABS_ERROR<T>))
                {
                        error("Regular simplex is not origin-centered");
                }
        }

        const T distance = (vertices[0] - vertices[1]).norm();
        for (unsigned i = 0; i < vertices.size(); ++i)
        {
                for (unsigned j = i + 1; j < vertices.size(); ++j)
                {
                        if (!(std::abs(distance - (vertices[i] - vertices[j]).norm()) < ABS_ERROR<T>))
                        {
                                error("Regular simplex vertices are not equidistant");
                        }
                }
        }
}

template <std::size_t N, typename T>
void test_facet_equal_distances(const std::string& name, const std::vector<std::array<Vector<N, T>, N>>& facets)
{
        static_assert(N >= 2);

        for (const std::array<Vector<N, T>, N>& vertices : facets)
        {
                const T d = (vertices[0] - vertices[1]).norm();
                for (unsigned i = 0; i < vertices.size(); ++i)
                {
                        for (unsigned j = i + 1; j < vertices.size(); ++j)
                        {
                                if (!(std::abs(d - (vertices[j] - vertices[i]).norm()) < ABS_ERROR<T>))
                                {
                                        error(name + " facet is not a simplex with equidistant vertices");
                                }
                        }
                }
        }
}

template <std::size_t N, typename T>
void test_unit_distance_from_origin(const std::string& name, const std::vector<std::array<Vector<N, T>, N>>& facets)
{
        for (const std::array<Vector<N, T>, N>& vertices : facets)
        {
                for (const Vector<N, T>& v : vertices)
                {
                        if (!(std::abs(1 - v.norm()) < ABS_ERROR<T>))
                        {
                                error(name + " is not origin-centered");
                        }
                }
        }
}

template <std::size_t N, typename T>
void test_facet_count(
        const std::string& name,
        const std::vector<std::array<Vector<N, T>, N>>& facets,
        unsigned facet_count)
{
        if (facets.size() != facet_count)
        {
                error(name + " facet count " + to_string(facets.size()) + " is not equal to " + to_string(facet_count));
        }
}

template <std::size_t N, typename T>
void test_vertex_count(
        const std::string& name,
        const std::vector<std::array<Vector<N, T>, N>>& facets,
        unsigned vertex_count)
{
        std::unordered_set<Vector<N, T>> vertex_set;
        for (const std::array<Vector<N, T>, N>& vertices : facets)
        {
                std::unordered_set<Vector<N, T>> facet_vertex_set;
                for (const Vector<N, T>& v : vertices)
                {
                        vertex_set.insert(v);
                        facet_vertex_set.insert(v);
                }
                if (facet_vertex_set.size() != N)
                {
                        error(name + " facet vertex count " + to_string(facet_vertex_set.size()) + " is not equal to "
                              + to_string(N));
                }
        }
        if (vertex_set.size() != vertex_count)
        {
                error(name + " vertex count " + to_string(vertex_set.size()) + " is not equal to "
                      + to_string(vertex_count));
        }
}

template <std::size_t N, typename T>
void test_euler_characteristic(const std::string& name, const std::vector<std::array<Vector<N, T>, N>>& facets)
{
        const std::vector<std::array<int, N>> mesh_facets = [&facets]()
        {
                std::vector<Vector<N, T>> tmp_vertices;
                std::vector<std::array<int, N>> tmp_facets;
                create_mesh(facets, &tmp_vertices, &tmp_facets);
                return tmp_facets;
        }();

        constexpr int EXPECTED_EULER_CHARACTERISTIC = euler_characteristic_for_convex_polytope<N>();

        const int computed_euler_characteristic = euler_characteristic(mesh_facets);

        if (computed_euler_characteristic == EXPECTED_EULER_CHARACTERISTIC)
        {
                return;
        }

        std::ostringstream oss;

        oss << name << " Euler characteristic (" << computed_euler_characteristic << ")";
        oss << " is not equal to " << EXPECTED_EULER_CHARACTERISTIC;

        std::array<long long, N> counts = simplex_counts(mesh_facets);
        for (unsigned i = 0; i < N; ++i)
        {
                oss << '\n' << i << "-simplex count = " << counts[i];
        }

        error(oss.str());
}

template <std::size_t N, typename T>
void test_polytope(
        const std::string& name,
        const std::vector<std::array<Vector<N, T>, N>>& facets,
        unsigned facet_count,
        unsigned vertex_count)
{
        test_facet_count(name, facets, facet_count);
        test_vertex_count(name, facets, vertex_count);
        test_facet_equal_distances(name, facets);
        test_unit_distance_from_origin(name, facets);
        test_euler_characteristic(name, facets);
}

template <std::size_t N, typename T>
void test_polytopes()
{
        LOG("Test regular polytopes in " + space_name(N) + ", " + type_name<T>());
        {
                std::array<Vector<N, T>, N + 1> simplex = create_simplex<N, T>();
                test_simplex(simplex);
        }
        {
                const std::string NAME = "Regular cross-polytope";
                constexpr unsigned FACET_COUNT = 1 << N;
                constexpr unsigned VERTEX_COUNT = 2 * N;
                std::vector<std::array<Vector<N, T>, N>> facets = create_cross_polytope<N, T>();
                test_polytope(NAME, facets, FACET_COUNT, VERTEX_COUNT);
        }
        if constexpr (N == 3)
        {
                const std::string NAME = "Regular icosahedron";
                constexpr unsigned FACET_COUNT = 20;
                constexpr unsigned VERTEX_COUNT = 12;
                std::vector<std::array<Vector<3, T>, 3>> facets = create_icosahedron<T>();
                test_polytope(NAME, facets, FACET_COUNT, VERTEX_COUNT);
        }
}

template <std::size_t N>
void test_polytopes()
{
        test_polytopes<N, float>();
        test_polytopes<N, double>();
        test_polytopes<N, long double>();
}

void test()
{
        test_polytopes<2>();
        test_polytopes<3>();
        test_polytopes<4>();
        test_polytopes<5>();
        test_polytopes<6>();
        test_polytopes<7>();
        test_polytopes<8>();
        test_polytopes<9>();
}

TEST_SMALL("Regular Polytopes", test)
}
}
