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

#include <array>
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

template <std::size_t N>
class FacetSubdivision
{
        std::vector<std::array<int, 2>> m_midpoints;
        std::vector<std::array<int, N>> m_facets;

        std::string subdivision_string() const
        {
                if (N >= 10)
                {
                        return {};
                }
                std::string result;
                for (std::array<int, N> vertices : m_facets)
                {
                        if (!result.empty())
                        {
                                result += '\n';
                        }
                        std::sort(vertices.begin(), vertices.end());
                        std::string s;
                        for (unsigned v : vertices)
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
                                        s += to_string(m_midpoints[v - N][1]);
                                }
                        }
                        result += s;
                }
                return result;
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
                                LOG(to_string(vertices.size()) + ": " + to_string(i) + ", " + to_string(j));
                                vertices.push_back((vertices[i] + vertices[j]).normalized());
                                m_midpoints.push_back({static_cast<int>(i), static_cast<int>(j)});
                        }
                }

                vertices.emplace_back(0);

                auto plane_facet = [&](const std::array<int, N>& facet_vertices)
                {
                        Vector<N, float> sum(0);
                        for (unsigned i : facet_vertices)
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
                };

                std::vector<ConvexHullFacet<N>> facets;
                ProgressRatio progress(nullptr);

                compute_convex_hull(vertices, &facets, &progress);

                for (const ConvexHullFacet<N>& facet : facets)
                {
                        if (!plane_facet(facet.vertices()))
                        {
                                m_facets.push_back(facet.vertices());
                        }
                }

                LOG("Facet subdivision\n" + subdivision_string());
        }

        const std::vector<std::array<int, 2>>& midpoints() const
        {
                return m_midpoints;
        }

        const std::vector<std::array<int, N>>& facets() const
        {
                return m_facets;
        }
};

template <std::size_t N, typename T>
struct Simplex
{
        std::array<Vector<N, T>, N> vertices;

        Simplex()
        {
        }

        explicit Simplex(const std::array<Vector<N, T>, N>& vertices) : vertices(vertices)
        {
        }

        template <typename... Ts>
        explicit Simplex(Ts... vertices) : vertices{vertices...}
        {
                static_assert(sizeof...(Ts) == N);
        }
};

//template <typename T>
//void divide_simplex(const Simplex<3, T>& simplex, std::vector<Simplex<3, T>>* simplices)
//{
//        const Vector<3, T>& v0 = simplex.vertices[0];
//        const Vector<3, T>& v1 = simplex.vertices[1];
//        const Vector<3, T>& v2 = simplex.vertices[2];
//        Vector<3, T> p01 = (v0 + v1).normalized();
//        Vector<3, T> p12 = (v1 + v2).normalized();
//        Vector<3, T> p20 = (v2 + v0).normalized();
//        simplices->emplace_back(v0, p01, p20);
//        simplices->emplace_back(v1, p12, p01);
//        simplices->emplace_back(v2, p20, p12);
//        simplices->emplace_back(p01, p12, p20);
//}

//template <typename T>
//void divide_simplex(const Simplex<4, T>& simplex, std::vector<Simplex<4, T>>* simplices)
//{
//        const Vector<4, T>& v0 = simplex.vertices[0];
//        const Vector<4, T>& v1 = simplex.vertices[1];
//        const Vector<4, T>& v2 = simplex.vertices[2];
//        const Vector<4, T>& v3 = simplex.vertices[3];
//        const Vector<4, T>& p01 = (v0 + v1).normalized();
//        const Vector<4, T>& p02 = (v0 + v2).normalized();
//        const Vector<4, T>& p03 = (v0 + v3).normalized();
//        const Vector<4, T>& p12 = (v1 + v2).normalized();
//        const Vector<4, T>& p13 = (v1 + v3).normalized();
//        const Vector<4, T>& p23 = (v2 + v3).normalized();
//        simplices->emplace_back(v0, p01, p02, p03);
//        simplices->emplace_back(v1, p01, p12, p13);
//        simplices->emplace_back(v2, p02, p12, p23);
//        simplices->emplace_back(v3, p03, p13, p23);
//        simplices->emplace_back(p01, p23, p02, p03);
//        simplices->emplace_back(p01, p23, p12, p13);
//        simplices->emplace_back(p01, p23, p02, p12);
//        simplices->emplace_back(p01, p23, p13, p03);
//}

template <std::size_t N, typename T>
std::vector<Simplex<N, T>> divide_simplices(
        unsigned min_count,
        const FacetSubdivision<N>& facet_subdivision,
        std::vector<Simplex<N, T>> simplices)
{
        if (simplices.empty())
        {
                return {};
        }

        std::vector<Vector<N, T>> points;

        while (simplices.size() < min_count)
        {
                std::vector<Simplex<N, T>> tmp;
                tmp.reserve(simplices.size() * facet_subdivision.facets().size());

                for (const Simplex<N, T>& simplex : simplices)
                {
                        points.clear();
                        for (const Vector<N, T>& v : simplex.vertices)
                        {
                                points.push_back(v);
                        }
                        for (const std::array<int, 2>& m : facet_subdivision.midpoints())
                        {
                                points.push_back((simplex.vertices[m[0]] + simplex.vertices[m[1]]).normalized());
                        }
                        for (const std::array<int, N>& indices : facet_subdivision.facets())
                        {
                                Simplex<N, T>& s = tmp.emplace_back();
                                for (unsigned i = 0; i < N; ++i)
                                {
                                        ASSERT(indices[i] < static_cast<int>(points.size()));
                                        s.vertices[i] = points[indices[i]];
                                }
                        }
                }
                simplices = std::move(tmp);
        }
        return simplices;
}

template <unsigned I, std::size_t N, typename T>
void create_simplices(Vector<N, T>* point, std::vector<Simplex<N, T>>* simplices)
{
        static_assert(I <= N);
        if constexpr (I == N)
        {
                std::array<Vector<N, T>, N> vertices;
                for (Vector<N, T>& v : vertices)
                {
                        v = Vector<N, T>(0);
                }
                for (unsigned i = 0; i < N; ++i)
                {
                        vertices[i][i] = (*point)[i];
                }
                simplices->emplace_back(vertices);
        }
        else
        {
                (*point)[I] = -1;
                create_simplices<I + 1>(point, simplices);
                (*point)[I] = 1;
                create_simplices<I + 1>(point, simplices);
        }
}

template <std::size_t N, typename T>
std::enable_if_t<N >= 4, std::vector<Simplex<N, T>>> create_initial_shape()
{
        // Cross-polytope

        std::vector<Simplex<N, T>> simplices;
        simplices.reserve(power<N>(2));
        Vector<N, T> point;
        create_simplices<0>(&point, &simplices);
        ASSERT(simplices.size() == power<N>(2));
        return simplices;
}

template <std::size_t N, typename T>
std::enable_if_t<N == 3, std::vector<Simplex<N, T>>> create_initial_shape()
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

        std::vector<Simplex<N, T>> facets;

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
std::vector<Simplex<N, T>> create_sphere(unsigned facet_min_count)
{
        std::vector<Simplex<N, T>> simplices = create_initial_shape<N, T>();

        for (Simplex<N, T>& s : simplices)
        {
                if (!(equal_distances(s.vertices) && equal_distance_from_origin(s.vertices)))
                {
                        error("Error creating initial shape");
                }
        }

        return divide_simplices(facet_min_count, FacetSubdivision<N>(), std::move(simplices));
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
void create_mesh_for_simplices(
        const std::vector<Simplex<N, T>>& simplices,
        std::vector<Vector<N, T>>* vertices,
        std::vector<std::array<int, N>>* facets)
{
        vertices->clear();
        facets->clear();
        facets->reserve(simplices.size());

        std::unordered_map<Vector<N, T>, unsigned> map;
        map.reserve(N * simplices.size());

        for (const Simplex<N, T>& simplex : simplices)
        {
                std::array<int, N>& facet = facets->emplace_back();
                for (unsigned i = 0; i < N; ++i)
                {
                        auto iter = map.find(simplex.vertices[i]);
                        if (iter == map.cend())
                        {
                                iter = map.emplace(simplex.vertices[i], vertices->size()).first;
                                vertices->push_back(simplex.vertices[i]);
                        }
                        facet[i] = iter->second;
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

        std::vector<impl::Simplex<N, T>> simplices = impl::create_sphere<N, T>(facet_min_count);

        impl::create_mesh_for_simplices(simplices, vertices, facets);

        impl::check_manifold(*facets);

        LOG("Sphere: vertex count = " + to_string(vertices->size()) + ", facet count = " + to_string(facets->size()));
}
}
