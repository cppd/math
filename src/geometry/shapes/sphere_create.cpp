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

#include "sphere_create.h"

#include "mesh.h"
#include "regular_polytopes.h"

#include "../core/convex_hull.h"

#include <src/numerical/vector.h>
#include <src/progress/progress.h>
#include <src/settings/instantiation.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <unordered_set>
#include <vector>

namespace ns::geometry::shapes
{
namespace
{
// template <std::size_t N, typename T>
// struct Facet final
// {
//         std::array<Vector<N, T>, N> vertices;
//
//         Facet()
//         {
//         }
//
//         explicit Facet(const std::array<Vector<N, T>, N>& vertices)
//                 : vertices(vertices)
//         {
//         }
//
//         template <typename... Ts>
//         explicit Facet(Ts... vertices)
//                 : vertices{vertices...}
//         {
//                 static_assert(sizeof...(Ts) == N);
//         }
// };
//
// template <std::size_t N>
// class FacetSubdivision final
// {
//         static_assert(N >= 4);
//
//         static constexpr std::size_t MIDPOINT_COUNT = (N * (N - 1)) / 2;
//
//         std::vector<std::array<int, N>> facets_;
//
//         template <std::size_t M>
//         static bool on_plane(const std::array<int, M>& object_indices, const std::vector<Vector<N, float>>& vertices)
//         {
//                 Vector<N, float> sum(0);
//                 for (const unsigned i : object_indices)
//                 {
//                         ASSERT(i < vertices.size());
//                         sum += vertices[i];
//                 }
//                 for (std::size_t i = 0; i < N; ++i)
//                 {
//                         if (sum[i] == 0)
//                         {
//                                 return true;
//                         }
//                 }
//                 return false;
//         }
//
//         template <std::size_t M>
//         static std::string vertex_indices_to_string(
//                 std::array<int, M> vertex_indices,
//                 const std::vector<std::string>& vertex_names)
//         {
//                 std::sort(vertex_indices.begin(), vertex_indices.end());
//                 std::string s;
//                 for (const unsigned v : vertex_indices)
//                 {
//                         if (!s.empty())
//                         {
//                                 s += ", ";
//                         }
//                         s += vertex_names[v];
//                 }
//                 return s;
//         }
//
//         static void check(
//                 std::vector<std::array<int, N>> facets,
//                 const std::vector<Vector<N, float>>& vertices,
//                 const std::vector<std::string>& vertex_names)
//         {
//                 ASSERT(vertices.size() == N + MIDPOINT_COUNT);
//
//                 for (std::array<int, N>& facet : facets)
//                 {
//                         std::sort(facet.begin(), facet.end());
//                 }
//                 std::sort(facets.begin(), facets.end());
//
//                 std::map<std::array<int, N - 1>, int> boundary_ridges;
//                 std::map<std::array<int, N - 1>, int> internal_ridges;
//                 std::map<std::array<int, N - 1>, int> ridges;
//
//                 for (const std::array<int, N>& facet : facets)
//                 {
//                         for (std::size_t r = 0; r < N; ++r)
//                         {
//                                 ASSERT(facet[r] < static_cast<int>(vertices.size()));
//                                 std::array<int, N - 1> ridge = sort(del_elem(facet, r));
//                                 if (on_plane(ridge, vertices))
//                                 {
//                                         ++boundary_ridges[ridge];
//                                 }
//                                 else
//                                 {
//                                         ++internal_ridges[ridge];
//                                 }
//                                 ++ridges[ridge];
//                                 ASSERT(ridges[ridge] <= 2);
//                         }
//                 }
//
//                 ASSERT(boundary_ridges.size() % N == 0);
//                 ASSERT(boundary_ridges.size() > N * (N - 1));
//                 ASSERT((boundary_ridges.size() - N * (N - 1)) % N == 0);
//
//                 std::ostringstream oss;
//
//                 oss << "Facet subdivisions " << facets.size();
//                 for (const std::array<int, N>& facet : facets)
//                 {
//                         oss << '\n' << vertex_indices_to_string(facet, vertex_names);
//                 }
//
//                 oss << '\n' << "Boundary ridges " << boundary_ridges.size();
//                 for (const auto& [ridge, count] : boundary_ridges)
//                 {
//                         ASSERT(count == 1);
//                         oss << '\n' << vertex_indices_to_string(ridge, vertex_names);
//                 }
//
//                 oss << '\n' << "Internal ridges " << internal_ridges.size();
//                 for (const auto& [ridge, count] : internal_ridges)
//                 {
//                         ASSERT(count == 2);
//                         oss << '\n' << vertex_indices_to_string(ridge, vertex_names);
//                 }
//
//                 LOG(oss.str());
//         }
//
// public:
//         FacetSubdivision()
//         {
//                 std::vector<Vector<N, float>> vertices;
//                 std::vector<std::string> vertex_names;
//
//                 for (std::size_t i = 0; i < N; ++i)
//                 {
//                         vertices.emplace_back(0);
//                         vertices.back()[i] = 1;
//                         vertex_names.push_back(to_string(i));
//                 }
//                 for (std::size_t i = 0; i < N; ++i)
//                 {
//                         for (std::size_t j = i + 1; j < N; ++j)
//                         {
//                                 vertices.push_back((vertices[i] + vertices[j]).normalized());
//                                 vertex_names.push_back(to_string(i) + (N <= 10 ? "" : "_") + to_string(j));
//                         }
//                 }
//
//                 vertices.emplace_back(0);
//
//                 ASSERT(vertices.size() == N + MIDPOINT_COUNT + 1);
//
//                 progress::Ratio progress(nullptr);
//
//                 const std::vector<core::ConvexHullSimplex<N>> facets =
//                         core::compute_convex_hull(vertices, &facets, &progress, false);
//
//                 for (const core::ConvexHullSimplex<N>& facet : facets)
//                 {
//                         if (!on_plane(facet.vertices(), vertices))
//                         {
//                                 facets_.push_back(facet.vertices());
//                         }
//                 }
//
//                 vertices.resize(vertices.size() - 1);
//
//                 check(facets_, vertices, vertex_names);
//         }
//
//         std::size_t facet_count() const
//         {
//                 return facets_.size();
//         }
//
//         template <typename T>
//         void divide(const Facet<N, T>& facet, std::vector<Facet<N, T>>* const facets) const
//         {
//                 std::array<Vector<N, T>, N + MIDPOINT_COUNT> points;
//
//                 unsigned index = 0;
//                 for (std::size_t i = 0; i < N; ++i)
//                 {
//                         points[index++] = facet.vertices[i];
//                 }
//                 for (std::size_t i = 0; i < N; ++i)
//                 {
//                         for (std::size_t j = i + 1; j < N; ++j)
//                         {
//                                 points[index++] = (points[i] + points[j]).normalized();
//                         }
//                 }
//                 ASSERT(index == points.size());
//                 for (const std::array<int, N>& indices : facets_)
//                 {
//                         Facet<N, T>& f = facets->emplace_back();
//                         for (std::size_t i = 0; i < N; ++i)
//                         {
//                                 ASSERT(indices[i] < static_cast<int>(points.size()));
//                                 f.vertices[i] = points[indices[i]];
//                         }
//                 }
//         }
// };

template <typename T>
void divide_facets(
        const unsigned min_facet_count,
        std::vector<std::array<Vector<3, T>, 3>> facets,
        std::vector<Vector<3, T>>* const mesh_vertices,
        std::vector<std::array<int, 3>>* const mesh_facets)
{
        for (std::array<Vector<3, T>, 3>& vertices : facets)
        {
                vertices[0].normalize();
                vertices[1].normalize();
                vertices[2].normalize();
        }

        while (facets.size() < min_facet_count)
        {
                std::vector<std::array<Vector<3, T>, 3>> tmp;
                tmp.reserve(4 * facets.size());

                for (const std::array<Vector<3, T>, 3>& vertices : facets)
                {
                        const Vector<3, T>& v0 = vertices[0];
                        const Vector<3, T>& v1 = vertices[1];
                        const Vector<3, T>& v2 = vertices[2];
                        const Vector<3, T> p01 = (v0 + v1).normalized();
                        const Vector<3, T> p12 = (v1 + v2).normalized();
                        const Vector<3, T> p20 = (v2 + v0).normalized();
                        tmp.push_back({v0, p01, p20});
                        tmp.push_back({v1, p12, p01});
                        tmp.push_back({v2, p20, p12});
                        tmp.push_back({p01, p12, p20});
                }

                facets = std::move(tmp);
        }

        create_mesh(facets, mesh_vertices, mesh_facets);
}

template <std::size_t N, typename T>
std::unordered_set<Vector<N, float>> create_initial_vertex_set(const std::vector<std::array<Vector<N, T>, N>>& facets)
{
        static_assert(N >= 4);

        std::unordered_set<Vector<N, float>> res;
        for (const std::array<Vector<N, T>, N>& vertices : facets)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        res.insert(to_vector<float>(vertices[i].normalized()));
                }
                for (std::size_t i = 0; i < N; ++i)
                {
                        for (std::size_t j = i + 1; j < N; ++j)
                        {
                                const Vector<N, T>& v1 = vertices[i];
                                const Vector<N, T>& v2 = vertices[j];
                                res.insert(to_vector<float>((v1 + v2).normalized()));
                        }
                }
        }
        return res;
}

template <std::size_t N>
void add_vertices(
        const std::vector<core::ConvexHullSimplex<N>>& facets,
        std::vector<Vector<N, float>>* const vertices,
        std::unordered_set<Vector<N, float>>* const vertex_set)
{
        static_assert(N >= 4);

        for (const core::ConvexHullSimplex<N>& facet : facets)
        {
                for (std::size_t i = 0; i < N; ++i)
                {
                        for (std::size_t j = i + 1; j < N; ++j)
                        {
                                const Vector<N, float>& v1 = (*vertices)[facet.vertices()[i]];
                                const Vector<N, float>& v2 = (*vertices)[facet.vertices()[j]];
                                const Vector<N, float> v = (v1 + v2).normalized();
                                if (vertex_set->insert(v).second)
                                {
                                        vertices->push_back(v);
                                }
                        }
                }
        }
}

template <std::size_t N, typename T>
        requires (N >= 4)
void divide_facets(
        const unsigned min_facet_count,
        const std::vector<std::array<Vector<N, T>, N>>& facets,
        std::vector<Vector<N, T>>* const mesh_vertices,
        std::vector<std::array<int, N>>* const mesh_facets)
{
        if (facets.size() >= min_facet_count)
        {
                create_mesh(facets, mesh_vertices, mesh_facets);
                return;
        }

        std::unordered_set<Vector<N, float>> vertex_set = create_initial_vertex_set(facets);
        std::vector<Vector<N, float>> ch_vertices(vertex_set.cbegin(), vertex_set.cend());
        std::vector<core::ConvexHullSimplex<N>> ch_facets;

        while (true)
        {
                progress::Ratio progress(nullptr);

                ch_facets = core::compute_convex_hull(ch_vertices, &progress, false);

                if (ch_facets.size() >= min_facet_count)
                {
                        break;
                }

                add_vertices(ch_facets, &ch_vertices, &vertex_set);
        }

        mesh_vertices->clear();
        mesh_vertices->reserve(ch_vertices.size());
        for (const Vector<N, float>& v : ch_vertices)
        {
                mesh_vertices->push_back(to_vector<T>(v));
        }

        mesh_facets->clear();
        mesh_facets->reserve(ch_facets.size());
        for (const core::ConvexHullSimplex<N>& ch_facet : ch_facets)
        {
                mesh_facets->push_back(ch_facet.vertices());
        }
}

template <std::size_t N, typename T>
std::vector<std::array<Vector<N, T>, N>> create_initial_facets()
{
        if constexpr (N >= 4)
        {
                return create_cross_polytope<N, T>();
        }
        else
        {
                return create_icosahedron<T>();
        }
}
}

template <std::size_t N, typename T>
void create_sphere(
        const unsigned min_facet_count,
        std::vector<Vector<N, T>>* const vertices,
        std::vector<std::array<int, N>>* const facets)
{
        divide_facets(min_facet_count, create_initial_facets<N, T>(), vertices, facets);
}

#define TEMPLATE(N, T) \
        template void create_sphere(unsigned, std::vector<Vector<(N), T>>*, std::vector<std::array<int, (N)>>*);

TEMPLATE_INSTANTIATION_N_T(TEMPLATE)
}
