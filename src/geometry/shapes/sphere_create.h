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
struct Simplex
{
        std::array<Vector<N, T>, N> vertices;

        explicit Simplex(const std::array<Vector<N, T>, N>& vertices) : vertices(vertices)
        {
        }

        template <typename... Ts>
        explicit Simplex(Ts... vertices) : vertices{vertices...}
        {
                static_assert(sizeof...(Ts) == N);
        }
};

template <std::size_t N>
constexpr std::enable_if_t<N == 3, unsigned> division_count()
{
        return 4;
}

template <typename T>
void divide_simplex(const Simplex<3, T>& simplex, std::vector<Simplex<3, T>>* simplices)
{
        Vector<3, T> p01 = (simplex.vertices[0] + simplex.vertices[1]).normalized();
        Vector<3, T> p12 = (simplex.vertices[1] + simplex.vertices[2]).normalized();
        Vector<3, T> p20 = (simplex.vertices[2] + simplex.vertices[0]).normalized();
        simplices->emplace_back(p01, p12, p20);
        simplices->emplace_back(simplex.vertices[0], p01, p20);
        simplices->emplace_back(simplex.vertices[1], p12, p01);
        simplices->emplace_back(simplex.vertices[2], p20, p12);
}

template <std::size_t N, typename T>
std::vector<Simplex<N, T>> divide_simplices(unsigned count, std::vector<Simplex<N, T>> simplices)
{
        while (simplices.size() < count)
        {
                std::vector<Simplex<N, T>> tmp;
                tmp.reserve(simplices.size() * division_count<N>());
                for (const Simplex<N, T>& facet : simplices)
                {
                        divide_simplex(facet, &tmp);
                }
                simplices = std::move(tmp);
        }
        return simplices;
}

template <std::size_t N, typename T>
std::enable_if_t<N == 3, std::vector<Simplex<N, T>>> create_sphere(unsigned count)
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

        std::vector<Simplex<N, T>> facets = {Simplex<N, T>(vertices[0], vertices[1], vertices[7]),
                                             Simplex<N, T>(vertices[0], vertices[5], vertices[1]),
                                             Simplex<N, T>(vertices[0], vertices[7], vertices[10]),
                                             Simplex<N, T>(vertices[0], vertices[10], vertices[11]),
                                             Simplex<N, T>(vertices[0], vertices[11], vertices[5]),
                                             Simplex<N, T>(vertices[1], vertices[5], vertices[9]),
                                             Simplex<N, T>(vertices[2], vertices[4], vertices[11]),
                                             Simplex<N, T>(vertices[3], vertices[2], vertices[6]),
                                             Simplex<N, T>(vertices[3], vertices[4], vertices[2]),
                                             Simplex<N, T>(vertices[3], vertices[6], vertices[8]),
                                             Simplex<N, T>(vertices[3], vertices[8], vertices[9]),
                                             Simplex<N, T>(vertices[3], vertices[9], vertices[4]),
                                             Simplex<N, T>(vertices[4], vertices[9], vertices[5]),
                                             Simplex<N, T>(vertices[5], vertices[11], vertices[4]),
                                             Simplex<N, T>(vertices[6], vertices[2], vertices[10]),
                                             Simplex<N, T>(vertices[7], vertices[1], vertices[8]),
                                             Simplex<N, T>(vertices[8], vertices[6], vertices[7]),
                                             Simplex<N, T>(vertices[9], vertices[8], vertices[1]),
                                             Simplex<N, T>(vertices[10], vertices[7], vertices[6]),
                                             Simplex<N, T>(vertices[11], vertices[10], vertices[2])};

        return divide_simplices(count, std::move(facets));
}

template <std::size_t N, typename T>
void create_mesh(
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
void create_sphere(unsigned facet_count, std::vector<Vector<N, T>>* vertices, std::vector<std::array<int, N>>* facets)
{
        namespace impl = sphere_implementation;

        std::vector<impl::Simplex<N, T>> simplices = impl::create_sphere<N, T>(facet_count);

        impl::create_mesh(simplices, vertices, facets);
}
}
