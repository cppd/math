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

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/numerical/vec.h>

#include <array>
#include <unordered_map>
#include <vector>

namespace ns::geometry
{
namespace sphere_implementation
{
//template <std::size_t N, typename T>
//bool equal_distances(const std::array<Vector<N, T>, N>& vertices)
//{
//        T d = -1;
//        for (unsigned i = 0; i < vertices.size(); ++i)
//        {
//                for (unsigned j = i + 1; j < vertices.size(); ++j)
//                {
//                        T s = 0;
//                        for (unsigned n = 0; n < N; ++n)
//                        {
//                                s += square(vertices[i][n] - vertices[j][n]);
//                        }
//                        s = std::sqrt(s);
//                        d = (d < 0) ? s : d;
//                        if (std::abs(s - d) > T(1e-2))
//                        {
//                                return false;
//                        }
//                }
//        }
//        return true;
//}

template <std::size_t N, typename T>
bool equal_distance_from_point(const std::vector<Vector<N, T>>& vertices, const Vector<N, T>& p)
{
        T d = -1;
        for (unsigned i = 0; i < vertices.size(); ++i)
        {
                T s = 0;
                for (unsigned n = 0; n < N; ++n)
                {
                        s += square(vertices[i][n] - p[n]);
                }
                s = std::sqrt(s);
                d = (d < 0) ? s : d;
                if (std::abs(s - d) > T(1e-5))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, typename T>
struct Facet
{
        std::array<Vector<N, T>, N> vertices;

        explicit Facet(const std::array<Vector<N, T>, N>& vertices) : vertices(vertices)
        {
        }

        template <typename... Ts>
        explicit Facet(Ts... vertices) : vertices{vertices...}
        {
                static_assert(sizeof...(Ts) == N);
        }
};

template <std::size_t N>
constexpr std::enable_if_t<N == 3, unsigned> divide_facet_count()
{
        return 4;
}

template <typename T>
void divide_facet(const Facet<3, T>& facet, std::vector<Facet<3, T>>* facets)
{
        Vector<3, T> p01 = (facet.vertices[0] + facet.vertices[1]).normalized();
        Vector<3, T> p12 = (facet.vertices[1] + facet.vertices[2]).normalized();
        Vector<3, T> p20 = (facet.vertices[2] + facet.vertices[0]).normalized();
        facets->emplace_back(p01, p12, p20);
        facets->emplace_back(facet.vertices[0], p01, p20);
        facets->emplace_back(facet.vertices[1], p12, p01);
        facets->emplace_back(facet.vertices[2], p20, p12);
}

template <std::size_t N, typename T>
std::vector<Facet<N, T>> divide_facets(unsigned count, std::vector<Facet<N, T>> facets)
{
        while (facets.size() < count)
        {
                std::vector<Facet<N, T>> tmp;
                tmp.reserve(facets.size() * divide_facet_count<N>());
                for (const Facet<N, T>& facet : facets)
                {
                        divide_facet(facet, &tmp);
                }
                facets = std::move(tmp);
        }
        return facets;
}

template <std::size_t N, typename T>
std::enable_if_t<N == 3, std::vector<Facet<N, T>>> create_sphere(unsigned count)
{
        static_assert(N == 3);

        const T p = (1 + std::sqrt(T(5))) / 2;

        std::vector<Vector<N, T>> vertices = {{-1, p, 0}, {1, p, 0}, {-1, -p, 0}, {1, -p, 0},
                                              {0, -1, p}, {0, 1, p}, {0, -1, -p}, {0, 1, -p},
                                              {p, 0, -1}, {p, 0, 1}, {-p, 0, -1}, {-p, 0, 1}};

        for (Vector<N, T>& v : vertices)
        {
                v.normalize();
        }

        ASSERT(equal_distance_from_point(vertices, Vector<N, T>(0)));

        std::vector<Facet<N, T>> facets = {Facet<N, T>(vertices[0], vertices[1], vertices[7]),
                                           Facet<N, T>(vertices[0], vertices[5], vertices[1]),
                                           Facet<N, T>(vertices[0], vertices[7], vertices[10]),
                                           Facet<N, T>(vertices[0], vertices[10], vertices[11]),
                                           Facet<N, T>(vertices[0], vertices[11], vertices[5]),
                                           Facet<N, T>(vertices[1], vertices[5], vertices[9]),
                                           Facet<N, T>(vertices[2], vertices[4], vertices[11]),
                                           Facet<N, T>(vertices[3], vertices[2], vertices[6]),
                                           Facet<N, T>(vertices[3], vertices[4], vertices[2]),
                                           Facet<N, T>(vertices[3], vertices[6], vertices[8]),
                                           Facet<N, T>(vertices[3], vertices[8], vertices[9]),
                                           Facet<N, T>(vertices[3], vertices[9], vertices[4]),
                                           Facet<N, T>(vertices[4], vertices[9], vertices[5]),
                                           Facet<N, T>(vertices[5], vertices[11], vertices[4]),
                                           Facet<N, T>(vertices[6], vertices[2], vertices[10]),
                                           Facet<N, T>(vertices[7], vertices[1], vertices[8]),
                                           Facet<N, T>(vertices[8], vertices[6], vertices[7]),
                                           Facet<N, T>(vertices[9], vertices[8], vertices[1]),
                                           Facet<N, T>(vertices[10], vertices[7], vertices[6]),
                                           Facet<N, T>(vertices[11], vertices[10], vertices[2])};

        return divide_facets(count, std::move(facets));
}
}

template <std::size_t N, typename T>
void create_sphere(unsigned facet_count, std::vector<Vector<N, T>>* points, std::vector<std::array<int, N>>* facets)
{
        namespace impl = sphere_implementation;

        std::vector<impl::Facet<N, T>> impl_facets = impl::create_sphere<N, T>(facet_count);

        points->clear();

        facets->clear();
        facets->reserve(impl_facets.size());

        std::unordered_map<Vector<N, T>, unsigned> map;
        map.reserve(N * impl_facets.size());

        for (const impl::Facet<N, T>& impl_f : impl_facets)
        {
                std::array<int, N>& vertices = facets->emplace_back();
                for (unsigned i = 0; i < N; ++i)
                {
                        auto iter = map.find(impl_f.vertices[i]);
                        if (iter == map.cend())
                        {
                                iter = map.emplace(impl_f.vertices[i], points->size()).first;
                                points->push_back(impl_f.vertices[i]);
                        }
                        vertices[i] = iter->second;
                }
        }
}
}
