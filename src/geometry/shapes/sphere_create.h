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

#pragma once

#include "../core/convex_hull.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/hash.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/sort.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <array>
#include <map>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace ns::geometry
{
namespace sphere_implementation
{
template <std::size_t N, typename T>
bool equal_distances(const std::array<Vector<N, T>, N>& vertices)
{
        if (vertices.size() < 2)
        {
                return true;
        }
        const T d = (vertices[0] - vertices[1]).norm();
        for (unsigned i = 0; i < vertices.size(); ++i)
        {
                for (unsigned j = i + 1; j < vertices.size(); ++j)
                {
                        if (std::abs(d - (vertices[j] - vertices[i]).norm()) > T(1e-3))
                        {
                                return false;
                        }
                }
        }
        return true;
}

template <std::size_t N, typename T>
bool equal_distance_from_origin(const std::array<Vector<N, T>, N>& vertices)
{
        if (vertices.empty())
        {
                return true;
        }
        const T d = vertices[0].norm();
        for (unsigned i = 1; i < vertices.size(); ++i)
        {
                if (std::abs(d - vertices[i].norm()) > T(1e-3))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, typename T>
struct Facet final
{
        std::array<Vector<N, T>, N> vertices;

        Facet()
        {
        }

        template <typename... Ts>
        explicit Facet(Ts... vertices) : vertices{vertices...}
        {
                static_assert(sizeof...(Ts) == N);
        }
};

template <std::size_t N>
class FacetSubdivision final
{
        static_assert(N >= 4);

        static constexpr std::size_t MIDPOINT_COUNT = (N * (N - 1)) / 2;

        std::vector<std::array<int, 2>> m_midpoints;
        std::vector<std::array<int, N>> m_facets;

        template <std::size_t M>
        static bool on_plane(const std::array<int, M>& object_indices, const std::vector<Vector<N, float>>& vertices)
        {
                Vector<N, float> sum(0);
                for (unsigned i : object_indices)
                {
                        ASSERT(i < vertices.size());
                        sum += vertices[i];
                }
                for (unsigned i = 0; i < N; ++i)
                {
                        if (sum[i] == 0)
                        {
                                return true;
                        }
                }
                return false;
        }

        template <std::size_t M>
        std::string indices_to_string(std::array<int, M> indices) const
        {
                std::sort(indices.begin(), indices.end());
                std::string s;
                for (unsigned v : indices)
                {
                        if (!s.empty())
                        {
                                s += ", ";
                        }
                        if (v < N)
                        {
                                s += to_string(v);
                        }
                        else
                        {
                                s += to_string(m_midpoints[v - N][0]);
                                if (N >= 10)
                                {
                                        s += '-';
                                }
                                s += to_string(m_midpoints[v - N][1]);
                        }
                }
                return s;
        }

        void check(const std::vector<Vector<N, float>>& vertices) const
        {
                ASSERT(vertices.size() == N + MIDPOINT_COUNT);

                std::vector<std::array<int, N>> facets = m_facets;
                for (std::array<int, N>& facet : facets)
                {
                        std::sort(facet.begin(), facet.end());
                }
                std::sort(facets.begin(), facets.end());

                std::map<std::array<int, N - 1>, int> boundary_ridges;
                std::map<std::array<int, N - 1>, int> internal_ridges;
                std::map<std::array<int, N - 1>, int> ridges;

                for (const std::array<int, N>& facet : facets)
                {
                        for (unsigned r = 0; r < N; ++r)
                        {
                                ASSERT(facet[r] < static_cast<int>(vertices.size()));
                                std::array<int, N - 1> ridge = sort(del_elem(facet, r));
                                if (on_plane(ridge, vertices))
                                {
                                        ++boundary_ridges[ridge];
                                }
                                else
                                {
                                        ++internal_ridges[ridge];
                                }
                                ++ridges[ridge];
                                ASSERT(ridges[ridge] <= 2);
                        }
                }

                ASSERT(boundary_ridges.size() % N == 0);
                ASSERT(boundary_ridges.size() > N * (N - 1));
                ASSERT((boundary_ridges.size() - N * (N - 1)) % N == 0);

                std::ostringstream oss;

                oss << "Facet subdivisions " << facets.size();
                for (const std::array<int, N>& facet : facets)
                {
                        oss << '\n' << indices_to_string(facet);
                }

                oss << '\n' << "Boundary ridges " << boundary_ridges.size();
                for (const auto& [ridge, count] : boundary_ridges)
                {
                        ASSERT(count == 1);
                        oss << '\n' << indices_to_string(ridge);
                }

                oss << '\n' << "Internal ridges " << internal_ridges.size();
                for (const auto& [ridge, count] : internal_ridges)
                {
                        ASSERT(count == 2);
                        oss << '\n' << indices_to_string(ridge);
                }

                LOG(oss.str());
        }

public:
        FacetSubdivision()
        {
                std::vector<Vector<N, float>> vertices;

                for (unsigned i = 0; i < N; ++i)
                {
                        vertices.emplace_back(0);
                        vertices.back()[i] = 1;
                }

                for (unsigned i = 0; i < N; ++i)
                {
                        for (unsigned j = i + 1; j < N; ++j)
                        {
                                vertices.push_back((vertices[i] + vertices[j]).normalized());
                                m_midpoints.push_back({static_cast<int>(i), static_cast<int>(j)});
                        }
                }
                ASSERT(m_midpoints.size() == MIDPOINT_COUNT);

                vertices.emplace_back(0);

                ASSERT(vertices.size() == N + MIDPOINT_COUNT + 1);

                std::vector<ConvexHullFacet<N>> facets;
                ProgressRatio progress(nullptr);

                compute_convex_hull(vertices, &facets, &progress);

                for (const ConvexHullFacet<N>& facet : facets)
                {
                        if (!on_plane(facet.vertices(), vertices))
                        {
                                m_facets.push_back(facet.vertices());
                        }
                }

                vertices.resize(vertices.size() - 1);

                check(vertices);
        }

        std::size_t facet_count() const
        {
                return m_facets.size();
        }

        template <typename T>
        void divide(const Facet<N, T>& facet, std::vector<Facet<N, T>>* facets) const
        {
                std::array<Vector<N, T>, N + MIDPOINT_COUNT> points;

                for (unsigned i = 0; i < N; ++i)
                {
                        points[i] = facet.vertices[i];
                }
                for (unsigned i = 0; i < m_midpoints.size(); ++i)
                {
                        int m0 = m_midpoints[i][0];
                        int m1 = m_midpoints[i][1];
                        points[N + i] = (facet.vertices[m0] + facet.vertices[m1]).normalized();
                }
                for (const std::array<int, N>& indices : m_facets)
                {
                        Facet<N, T>& f = facets->emplace_back();
                        for (unsigned i = 0; i < N; ++i)
                        {
                                ASSERT(indices[i] < static_cast<int>(points.size()));
                                f.vertices[i] = points[indices[i]];
                        }
                }
        }
};

template <>
class FacetSubdivision<3> final
{
public:
        std::size_t facet_count() const
        {
                return 4;
        }

        template <typename T>
        void divide(const Facet<3, T>& facet, std::vector<Facet<3, T>>* facets) const
        {
                const Vector<3, T>& v0 = facet.vertices[0];
                const Vector<3, T>& v1 = facet.vertices[1];
                const Vector<3, T>& v2 = facet.vertices[2];
                Vector<3, T> p01 = (v0 + v1).normalized();
                Vector<3, T> p12 = (v1 + v2).normalized();
                Vector<3, T> p20 = (v2 + v0).normalized();
                facets->emplace_back(v0, p01, p20);
                facets->emplace_back(v1, p12, p01);
                facets->emplace_back(v2, p20, p12);
                facets->emplace_back(p01, p12, p20);
        }
};

template <std::size_t N, typename T>
std::vector<Facet<N, T>> divide_facets(
        unsigned min_count,
        const FacetSubdivision<N>& facet_subdivision,
        std::vector<Facet<N, T>> facets)
{
        if (facets.empty())
        {
                return facets;
        }

        while (facets.size() < min_count)
        {
                std::vector<Facet<N, T>> tmp;
                tmp.reserve(facets.size() * facet_subdivision.facet_count());
                for (const Facet<N, T>& facet : facets)
                {
                        facet_subdivision.divide(facet, &tmp);
                }
                facets = std::move(tmp);
        }

        return facets;
}

template <unsigned I, std::size_t N, typename T>
void create_facets(Vector<N, T>* point, std::vector<Facet<N, T>>* facets)
{
        static_assert(I <= N);
        if constexpr (I == N)
        {
                Facet<N, T>& facet = facets->emplace_back();
                for (unsigned i = 0; i < N; ++i)
                {
                        facet.vertices[i] = Vector<N, T>(0);
                        facet.vertices[i][i] = (*point)[i];
                }
        }
        else
        {
                (*point)[I] = -1;
                create_facets<I + 1>(point, facets);
                (*point)[I] = 1;
                create_facets<I + 1>(point, facets);
        }
}

template <std::size_t N, typename T>
std::enable_if_t<N >= 4, std::vector<Facet<N, T>>> create_initial_shape()
{
        // Cross-polytope

        std::vector<Facet<N, T>> facets;
        facets.reserve(power<N>(2));
        Vector<N, T> point;
        create_facets<0>(&point, &facets);
        ASSERT(facets.size() == power<N>(2));
        return facets;
}

template <std::size_t N, typename T>
std::enable_if_t<N == 3, std::vector<Facet<N, T>>> create_initial_shape()
{
        // Regular icosahedron

        const T p = (1 + std::sqrt(T(5))) / 2;

        std::vector<Vector<N, T>> vertices;

        vertices.emplace_back(-1, p, 0);
        vertices.emplace_back(1, p, 0);
        vertices.emplace_back(-1, -p, 0);
        vertices.emplace_back(1, -p, 0);
        vertices.emplace_back(0, -1, p);
        vertices.emplace_back(0, 1, p);
        vertices.emplace_back(0, -1, -p);
        vertices.emplace_back(0, 1, -p);
        vertices.emplace_back(p, 0, -1);
        vertices.emplace_back(p, 0, 1);
        vertices.emplace_back(-p, 0, -1);
        vertices.emplace_back(-p, 0, 1);

        for (Vector<N, T>& v : vertices)
        {
                v.normalize();
        }

        std::vector<Facet<N, T>> facets;

        facets.emplace_back(vertices[0], vertices[1], vertices[7]);
        facets.emplace_back(vertices[0], vertices[5], vertices[1]);
        facets.emplace_back(vertices[0], vertices[7], vertices[10]);
        facets.emplace_back(vertices[0], vertices[10], vertices[11]);
        facets.emplace_back(vertices[0], vertices[11], vertices[5]);
        facets.emplace_back(vertices[1], vertices[5], vertices[9]);
        facets.emplace_back(vertices[2], vertices[4], vertices[11]);
        facets.emplace_back(vertices[3], vertices[2], vertices[6]);
        facets.emplace_back(vertices[3], vertices[4], vertices[2]);
        facets.emplace_back(vertices[3], vertices[6], vertices[8]);
        facets.emplace_back(vertices[3], vertices[8], vertices[9]);
        facets.emplace_back(vertices[3], vertices[9], vertices[4]);
        facets.emplace_back(vertices[4], vertices[9], vertices[5]);
        facets.emplace_back(vertices[5], vertices[11], vertices[4]);
        facets.emplace_back(vertices[6], vertices[2], vertices[10]);
        facets.emplace_back(vertices[7], vertices[1], vertices[8]);
        facets.emplace_back(vertices[8], vertices[6], vertices[7]);
        facets.emplace_back(vertices[9], vertices[8], vertices[1]);
        facets.emplace_back(vertices[10], vertices[7], vertices[6]);
        facets.emplace_back(vertices[11], vertices[10], vertices[2]);

        return facets;
}

template <std::size_t N, typename T>
std::vector<Facet<N, T>> create_sphere(unsigned facet_min_count)
{
        std::vector<Facet<N, T>> facets = create_initial_shape<N, T>();

        for (Facet<N, T>& facet : facets)
        {
                if (!(equal_distances(facet.vertices) && equal_distance_from_origin(facet.vertices)))
                {
                        error("Error creating initial shape");
                }
        }

        return divide_facets(facet_min_count, FacetSubdivision<N>(), std::move(facets));
}

template <std::size_t N>
void check_manifold(const std::vector<std::array<int, N>>& facets)
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
                        error("Error creating sphere: facet count " + to_string(count) + " is not equal to 2 for ridge "
                              + to_string(ridge));
                }
        }
}

template <std::size_t N, typename T>
void create_mesh(
        const std::vector<Facet<N, T>>& facets,
        std::vector<Vector<N, T>>* mesh_vertices,
        std::vector<std::array<int, N>>* mesh_facets)
{
        mesh_vertices->clear();
        mesh_facets->clear();
        mesh_facets->reserve(facets.size());

        std::unordered_map<Vector<N, T>, unsigned> map;
        map.reserve(N * facets.size());

        for (const Facet<N, T>& facet : facets)
        {
                std::array<int, N>& mesh_facet = mesh_facets->emplace_back();
                for (unsigned i = 0; i < N; ++i)
                {
                        auto iter = map.find(facet.vertices[i]);
                        if (iter == map.cend())
                        {
                                iter = map.emplace(facet.vertices[i], mesh_vertices->size()).first;
                                mesh_vertices->push_back(facet.vertices[i]);
                        }
                        mesh_facet[i] = iter->second;
                }
        }
}
}

template <std::size_t N, typename T>
void create_sphere(
        unsigned facet_min_count,
        std::vector<Vector<N, T>>* vertices,
        std::vector<std::array<int, N>>* facets)
{
        namespace impl = sphere_implementation;

        std::vector<impl::Facet<N, T>> vertex_facets = impl::create_sphere<N, T>(facet_min_count);

        impl::create_mesh(vertex_facets, vertices, facets);

        impl::check_manifold(*facets);

        LOG("Sphere: vertex count = " + to_string(vertices->size()) + ", facet count = " + to_string(facets->size()));
}
}
