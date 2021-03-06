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

#include "sphere_create.h"

#include "mesh.h"
#include "regular_polytopes.h"

#include "../core/convex_hull.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/sort.h>

#include <algorithm>
#include <map>
#include <sstream>
#include <unordered_set>

namespace ns::geometry
{
namespace
{
//template <std::size_t N, typename T>
//struct Facet final
//{
//        std::array<Vector<N, T>, N> vertices;
//
//        Facet()
//        {
//        }
//
//        explicit Facet(const std::array<Vector<N, T>, N>& vertices) : vertices(vertices)
//        {
//        }
//
//        template <typename... Ts>
//        explicit Facet(Ts... vertices) : vertices{vertices...}
//        {
//                static_assert(sizeof...(Ts) == N);
//        }
//};
//
//template <std::size_t N>
//class FacetSubdivision final
//{
//        static_assert(N >= 4);
//
//        static constexpr std::size_t MIDPOINT_COUNT = (N * (N - 1)) / 2;
//
//        std::vector<std::array<int, N>> m_facets;
//
//        template <std::size_t M>
//        static bool on_plane(const std::array<int, M>& object_indices, const std::vector<Vector<N, float>>& vertices)
//        {
//                Vector<N, float> sum(0);
//                for (unsigned i : object_indices)
//                {
//                        ASSERT(i < vertices.size());
//                        sum += vertices[i];
//                }
//                for (unsigned i = 0; i < N; ++i)
//                {
//                        if (sum[i] == 0)
//                        {
//                                return true;
//                        }
//                }
//                return false;
//        }
//
//        template <std::size_t M>
//        static std::string vertex_indices_to_string(
//                std::array<int, M> vertex_indices,
//                const std::vector<std::string>& vertex_names)
//        {
//                std::sort(vertex_indices.begin(), vertex_indices.end());
//                std::string s;
//                for (unsigned v : vertex_indices)
//                {
//                        if (!s.empty())
//                        {
//                                s += ", ";
//                        }
//                        s += vertex_names[v];
//                }
//                return s;
//        }
//
//        static void check(
//                std::vector<std::array<int, N>> facets,
//                const std::vector<Vector<N, float>>& vertices,
//                const std::vector<std::string>& vertex_names)
//        {
//                ASSERT(vertices.size() == N + MIDPOINT_COUNT);
//
//                for (std::array<int, N>& facet : facets)
//                {
//                        std::sort(facet.begin(), facet.end());
//                }
//                std::sort(facets.begin(), facets.end());
//
//                std::map<std::array<int, N - 1>, int> boundary_ridges;
//                std::map<std::array<int, N - 1>, int> internal_ridges;
//                std::map<std::array<int, N - 1>, int> ridges;
//
//                for (const std::array<int, N>& facet : facets)
//                {
//                        for (unsigned r = 0; r < N; ++r)
//                        {
//                                ASSERT(facet[r] < static_cast<int>(vertices.size()));
//                                std::array<int, N - 1> ridge = sort(del_elem(facet, r));
//                                if (on_plane(ridge, vertices))
//                                {
//                                        ++boundary_ridges[ridge];
//                                }
//                                else
//                                {
//                                        ++internal_ridges[ridge];
//                                }
//                                ++ridges[ridge];
//                                ASSERT(ridges[ridge] <= 2);
//                        }
//                }
//
//                ASSERT(boundary_ridges.size() % N == 0);
//                ASSERT(boundary_ridges.size() > N * (N - 1));
//                ASSERT((boundary_ridges.size() - N * (N - 1)) % N == 0);
//
//                std::ostringstream oss;
//
//                oss << "Facet subdivisions " << facets.size();
//                for (const std::array<int, N>& facet : facets)
//                {
//                        oss << '\n' << vertex_indices_to_string(facet, vertex_names);
//                }
//
//                oss << '\n' << "Boundary ridges " << boundary_ridges.size();
//                for (const auto& [ridge, count] : boundary_ridges)
//                {
//                        ASSERT(count == 1);
//                        oss << '\n' << vertex_indices_to_string(ridge, vertex_names);
//                }
//
//                oss << '\n' << "Internal ridges " << internal_ridges.size();
//                for (const auto& [ridge, count] : internal_ridges)
//                {
//                        ASSERT(count == 2);
//                        oss << '\n' << vertex_indices_to_string(ridge, vertex_names);
//                }
//
//                LOG(oss.str());
//        }
//
//public:
//        FacetSubdivision()
//        {
//                std::vector<Vector<N, float>> vertices;
//                std::vector<std::string> vertex_names;
//
//                for (unsigned i = 0; i < N; ++i)
//                {
//                        vertices.emplace_back(0);
//                        vertices.back()[i] = 1;
//                        vertex_names.push_back(to_string(i));
//                }
//                for (unsigned i = 0; i < N; ++i)
//                {
//                        for (unsigned j = i + 1; j < N; ++j)
//                        {
//                                vertices.push_back((vertices[i] + vertices[j]).normalized());
//                                vertex_names.push_back(to_string(i) + (N <= 10 ? "" : "_") + to_string(j));
//                        }
//                }
//
//                vertices.emplace_back(0);
//
//                ASSERT(vertices.size() == N + MIDPOINT_COUNT + 1);
//
//                std::vector<ConvexHullFacet<N>> facets;
//                ProgressRatio progress(nullptr);
//
//                compute_convex_hull(vertices, &facets, &progress, false);
//
//                for (const ConvexHullFacet<N>& facet : facets)
//                {
//                        if (!on_plane(facet.vertices(), vertices))
//                        {
//                                m_facets.push_back(facet.vertices());
//                        }
//                }
//
//                vertices.resize(vertices.size() - 1);
//
//                check(m_facets, vertices, vertex_names);
//        }
//
//        std::size_t facet_count() const
//        {
//                return m_facets.size();
//        }
//
//        template <typename T>
//        void divide(const Facet<N, T>& facet, std::vector<Facet<N, T>>* facets) const
//        {
//                std::array<Vector<N, T>, N + MIDPOINT_COUNT> points;
//
//                unsigned index = 0;
//                for (unsigned i = 0; i < N; ++i)
//                {
//                        points[index++] = facet.vertices[i];
//                }
//                for (unsigned i = 0; i < N; ++i)
//                {
//                        for (unsigned j = i + 1; j < N; ++j)
//                        {
//                                points[index++] = (points[i] + points[j]).normalized();
//                        }
//                }
//                ASSERT(index == points.size());
//                for (const std::array<int, N>& indices : m_facets)
//                {
//                        Facet<N, T>& f = facets->emplace_back();
//                        for (unsigned i = 0; i < N; ++i)
//                        {
//                                ASSERT(indices[i] < static_cast<int>(points.size()));
//                                f.vertices[i] = points[indices[i]];
//                        }
//                }
//        }
//};

template <typename T>
void divide_facets(
        unsigned min_facet_count,
        std::vector<std::array<Vector<3, T>, 3>> facets,
        std::vector<Vector<3, T>>* mesh_vertices,
        std::vector<std::array<int, 3>>* mesh_facets)
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
                        Vector<3, T> p01 = (v0 + v1).normalized();
                        Vector<3, T> p12 = (v1 + v2).normalized();
                        Vector<3, T> p20 = (v2 + v0).normalized();
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
std::enable_if_t<N >= 4> divide_facets(
        unsigned min_facet_count,
        const std::vector<std::array<Vector<N, T>, N>>& facets,
        std::vector<Vector<N, T>>* mesh_vertices,
        std::vector<std::array<int, N>>* mesh_facets)
{
        if (facets.size() >= min_facet_count)
        {
                create_mesh(facets, mesh_vertices, mesh_facets);
                return;
        }

        std::unordered_set<Vector<N, float>> vertex_set;

        for (const std::array<Vector<N, T>, N>& vertices : facets)
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        vertex_set.insert(to_vector<float>(vertices[i].normalized()));
                }
                for (unsigned i = 0; i < N; ++i)
                {
                        for (unsigned j = i + 1; j < N; ++j)
                        {
                                const Vector<N, T>& v1 = vertices[i];
                                const Vector<N, T>& v2 = vertices[j];
                                vertex_set.insert(to_vector<float>((v1 + v2).normalized()));
                        }
                }
        }

        std::vector<Vector<N, float>> ch_vertices;
        std::vector<ConvexHullFacet<N>> ch_facets;

        ch_vertices.insert(ch_vertices.cend(), vertex_set.cbegin(), vertex_set.cend());

        while (true)
        {
                ProgressRatio progress(nullptr);

                compute_convex_hull(ch_vertices, &ch_facets, &progress, false);

                if (ch_facets.size() >= min_facet_count)
                {
                        break;
                }

                for (const ConvexHullFacet<N>& ch_facet : ch_facets)
                {
                        for (unsigned i = 0; i < N; ++i)
                        {
                                for (unsigned j = i + 1; j < N; ++j)
                                {
                                        const Vector<N, float>& v1 = ch_vertices[ch_facet.vertices()[i]];
                                        const Vector<N, float>& v2 = ch_vertices[ch_facet.vertices()[j]];
                                        Vector<N, float> v = (v1 + v2).normalized();
                                        if (vertex_set.insert(v).second)
                                        {
                                                ch_vertices.push_back(v);
                                        }
                                }
                        }
                }
        }

        mesh_vertices->clear();
        mesh_vertices->reserve(ch_vertices.size());
        for (const Vector<N, float>& v : ch_vertices)
        {
                mesh_vertices->push_back(to_vector<T>(v));
        }

        mesh_facets->clear();
        mesh_facets->reserve(ch_facets.size());
        for (const ConvexHullFacet<N>& ch_facet : ch_facets)
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
        unsigned facet_min_count,
        std::vector<Vector<N, T>>* vertices,
        std::vector<std::array<int, N>>* facets)
{
        divide_facets(facet_min_count, create_initial_facets<N, T>(), vertices, facets);
}

template void create_sphere(unsigned, std::vector<Vector<3, float>>*, std::vector<std::array<int, 3>>*);
template void create_sphere(unsigned, std::vector<Vector<4, float>>*, std::vector<std::array<int, 4>>*);
template void create_sphere(unsigned, std::vector<Vector<5, float>>*, std::vector<std::array<int, 5>>*);
template void create_sphere(unsigned, std::vector<Vector<6, float>>*, std::vector<std::array<int, 6>>*);
template void create_sphere(unsigned, std::vector<Vector<3, double>>*, std::vector<std::array<int, 3>>*);
template void create_sphere(unsigned, std::vector<Vector<4, double>>*, std::vector<std::array<int, 4>>*);
template void create_sphere(unsigned, std::vector<Vector<5, double>>*, std::vector<std::array<int, 5>>*);
template void create_sphere(unsigned, std::vector<Vector<6, double>>*, std::vector<std::array<int, 6>>*);
}
