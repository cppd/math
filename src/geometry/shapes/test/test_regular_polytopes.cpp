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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/names.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/com/type/name.h>
#include <src/geometry/core/check.h>
#include <src/geometry/core/euler.h>
#include <src/geometry/shapes/mesh.h>
#include <src/geometry/shapes/regular_polytopes.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <array>
#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

namespace ns::geometry::shapes::test
{
namespace
{
template <typename T>
constexpr T ABS_ERROR = 10 * Limits<T>::epsilon();

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
        for (std::size_t i = 0; i < vertices.size(); ++i)
        {
                for (std::size_t j = i + 1; j < vertices.size(); ++j)
                {
                        if (!(std::abs(distance - (vertices[i] - vertices[j]).norm()) < ABS_ERROR<T>))
                        {
                                error("Regular simplex vertices are not equidistant");
                        }
                }
        }
}

template <std::size_t N, typename T>
void check_facet_equal_distances(const std::string& name, const std::vector<std::array<Vector<N, T>, N>>& facets)
{
        static_assert(N >= 2);

        for (const std::array<Vector<N, T>, N>& vertices : facets)
        {
                const T d = (vertices[0] - vertices[1]).norm();
                for (std::size_t i = 0; i < vertices.size(); ++i)
                {
                        for (std::size_t j = i + 1; j < vertices.size(); ++j)
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
void check_unit_distance_from_origin(const std::string& name, const std::vector<std::array<Vector<N, T>, N>>& facets)
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
void check_facet_count(
        const std::string& name,
        const std::vector<std::array<Vector<N, T>, N>>& facets,
        const unsigned facet_count)
{
        if (facets.size() != facet_count)
        {
                error(name + " facet count " + to_string(facets.size()) + " is not equal to " + to_string(facet_count));
        }
}

template <std::size_t N, typename T>
void check_vertex_count(
        const std::string& name,
        const std::vector<std::array<Vector<N, T>, N>>& facets,
        const unsigned vertex_count)
{
        std::unordered_set<Vector<N, T>> vertex_set;
        for (const std::array<Vector<N, T>, N>& vertices : facets)
        {
                for (const Vector<N, T>& v : vertices)
                {
                        vertex_set.insert(v);
                }
        }
        if (vertex_set.size() != vertex_count)
        {
                error(name + " vertex count " + to_string(vertex_set.size()) + " is not equal to "
                      + to_string(vertex_count));
        }
}

template <std::size_t N, typename T>
void test_polytope(
        const std::string& name,
        const std::vector<std::array<Vector<N, T>, N>>& facets,
        const unsigned facet_count,
        const unsigned vertex_count)
{
        check_facet_count(name, facets, facet_count);
        check_vertex_count(name, facets, vertex_count);
        check_facet_equal_distances(name, facets);
        check_unit_distance_from_origin(name, facets);

        std::vector<Vector<N, T>> mesh_vertices;
        std::vector<std::array<int, N>> mesh_facets;
        create_mesh(facets, &mesh_vertices, &mesh_facets);

        constexpr bool HAS_BOUNDARY = false;
        core::check_mesh(
                name, mesh_vertices, mesh_facets, HAS_BOUNDARY, core::euler_characteristic_for_convex_polytope<N>());
}

template <std::size_t N, typename T>
void test_polytopes()
{
        LOG("Test regular polytopes in " + space_name(N) + ", " + type_name<T>());
        {
                const std::array<Vector<N, T>, N + 1> simplex = create_simplex<N, T>();
                test_simplex(simplex);
        }
        {
                const std::string name = "Regular cross-polytope";
                constexpr unsigned FACET_COUNT = 1 << N;
                constexpr unsigned VERTEX_COUNT = 2 * N;
                const std::vector<std::array<Vector<N, T>, N>> facets = create_cross_polytope<N, T>();
                test_polytope(name, facets, FACET_COUNT, VERTEX_COUNT);
        }
        if constexpr (N == 3)
        {
                const std::string name = "Regular icosahedron";
                constexpr unsigned FACET_COUNT = 20;
                constexpr unsigned VERTEX_COUNT = 12;
                const std::vector<std::array<Vector<3, T>, 3>> facets = create_icosahedron<T>();
                test_polytope(name, facets, FACET_COUNT, VERTEX_COUNT);
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
