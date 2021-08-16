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

#include "constraint.h"
#include "hyperplane.h"

#include <src/com/error.h>
#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <array>

namespace ns::geometry
{
template <std::size_t N, typename T>
class HyperplaneParallelotope final
{
        static_assert(N <= 30);
        static constexpr int VERTEX_COUNT = 1 << (N - 1);
        // Количество вершин 2 ^ (N-1) умножить на количество измерений (N-1) у каждой вершины
        // и для уникальности разделить на 2 = ((2 ^ (N-1)) * (N-1)) / 2 = (2 ^ (N-2)) * (N-1)
        static constexpr int EDGE_COUNT = (1 << (N - 2)) * (N - 1);

        struct Planes
        {
                Vector<N, T> n;
                T d;
        };
        std::array<Planes, N - 1> planes_;

        Vector<N, T> org_;
        std::array<Vector<N, T>, N - 1> vectors_;
        Vector<N, T> normal_;

        template <int INDEX, typename F>
        void vertices_impl(const Vector<N, T>& p, const F& f) const;

        template <int INDEX, typename F>
        void edges_impl(const Vector<N, T>& p, std::array<bool, N - 1>* dimensions, const F& f) const;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N - 1;

        using DataType = T;

        template <typename... P>
        explicit HyperplaneParallelotope(const Vector<N, T>& org, const P&... vectors);

        HyperplaneParallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors);

        Constraints<N, T, 2 * (N - 1), 1> constraints() const;

        std::optional<T> intersect(const Ray<N, T>& r) const;

        const Vector<N, T>& normal(const Vector<N, T>& point) const;

        std::array<Vector<N, T>, VERTEX_COUNT> vertices() const;

        std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> edges() const;

        const Vector<N, T>& org() const;
        const Vector<N, T>& e(unsigned n) const;
};

template <std::size_t N, typename T>
template <typename... P>
HyperplaneParallelotope<N, T>::HyperplaneParallelotope(const Vector<N, T>& org, const P&... vectors)
        : HyperplaneParallelotope(org, {vectors...})
{
        static_assert((std::is_same_v<Vector<N, T>, P> && ...));
        static_assert(sizeof...(P) + 1 == N);
}

template <std::size_t N, typename T>
HyperplaneParallelotope<N, T>::HyperplaneParallelotope(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N - 1>& vectors)
{
        org_ = org;
        vectors_ = vectors;
        normal_ = numerical::orthogonal_complement(vectors).normalized();

        for (unsigned i = 0; i < N - 1; ++i)
        {
                std::swap(normal_, vectors_[i]);
                planes_[i].n = numerical::orthogonal_complement(vectors_);
                std::swap(normal_, vectors_[i]);

                if (dot(planes_[i].n, vectors_[i]) < 0)
                {
                        planes_[i].n = -planes_[i].n;
                }

                planes_[i].d = dot(org_, planes_[i].n);

                // Относительное расстояние от вершины до плоскости должно быть равно 1
                T distance = dot(org_ + vectors_[i], planes_[i].n) - planes_[i].d;
                ASSERT(distance >= 0);
                planes_[i].n /= distance;
                planes_[i].d /= distance;
        }
}

// 2*(N-1) неравенств в виде b + a * x >= 0 и одно равенство в виде b + a * x = 0
template <std::size_t N, typename T>
Constraints<N, T, 2 * (N - 1), 1> HyperplaneParallelotope<N, T>::constraints() const
{
        Constraints<N, T, 2 * (N - 1), 1> result;

        // Плоскости n * x - d имеют перпендикуляр с направлением наружу.
        // Направление внутрь -n * x + d или d + -(n * x), тогда условие
        // для точек параллелотопа d + -(n * x) >= 0.
        for (unsigned i = 0, c_i = 0; i < N - 1; ++i, c_i += 2)
        {
                T len = planes_[i].n.norm();

                result.c[c_i].a = planes_[i].n / len;
                result.c[c_i].b = -planes_[i].d / len;

                result.c[c_i + 1].a = -planes_[i].n / len;
                result.c[c_i + 1].b = dot(org_ + vectors_[i], planes_[i].n) / len;
        }

        result.c_eq[0].a = normal_;
        result.c_eq[0].b = -dot(org_, normal_);

        return result;
}

template <std::size_t N, typename T>
std::optional<T> HyperplaneParallelotope<N, T>::intersect(const Ray<N, T>& r) const
{
        std::optional<T> t = hyperplane_intersect(r, org_, normal_);
        if (!t)
        {
                return std::nullopt;
        }

        Vector<N, T> intersection_point = r.point(*t);

        for (unsigned i = 0; i < N - 1; ++i)
        {
                // Относительное расстояние от грани до точки является координатой точки
                T d = dot(intersection_point, planes_[i].n) - planes_[i].d;
                if (d <= 0 || d >= 1)
                {
                        return std::nullopt;
                }
        }

        return t;
}

template <std::size_t N, typename T>
const Vector<N, T>& HyperplaneParallelotope<N, T>::normal(const Vector<N, T>&) const
{
        return normal_;
}

template <std::size_t N, typename T>
template <int INDEX, typename F>
void HyperplaneParallelotope<N, T>::vertices_impl(const Vector<N, T>& p, const F& f) const
{
        if constexpr (INDEX >= 0)
        {
                vertices_impl<INDEX - 1>(p, f);
                vertices_impl<INDEX - 1>(p + vectors_[INDEX], f);
        }
        else
        {
                f(p);
        }
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, HyperplaneParallelotope<N, T>::VERTEX_COUNT> HyperplaneParallelotope<N, T>::vertices() const
{
        std::array<Vector<N, T>, VERTEX_COUNT> result;

        unsigned count = 0;
        auto f = [&count, &result](const Vector<N, T>& p)
        {
                ASSERT(count < result.size());
                result[count++] = p;
        };

        vertices_impl<N - 2>(org_, f);

        ASSERT(count == result.size());

        return result;
}

template <std::size_t N, typename T>
template <int INDEX, typename F>
void HyperplaneParallelotope<N, T>::edges_impl(const Vector<N, T>& p, std::array<bool, N - 1>* dimensions, const F& f)
        const
{
        static_assert(N <= 3);

        if constexpr (INDEX >= 0)
        {
                (*dimensions)[INDEX] = true;
                edges_impl<INDEX - 1>(p, dimensions, f);

                (*dimensions)[INDEX] = false;
                edges_impl<INDEX - 1>(p + vectors_[INDEX], dimensions, f);
        }
        else
        {
                f(p);
        }
}

template <std::size_t N, typename T>
std::array<std::array<Vector<N, T>, 2>, HyperplaneParallelotope<N, T>::EDGE_COUNT> HyperplaneParallelotope<N, T>::
        edges() const
{
        static_assert(N <= 3);

        std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> result;

        unsigned count = 0;
        std::array<bool, N - 1> dimensions;
        auto f = [this, &dimensions, &count, &result](const Vector<N, T>& p)
        {
                for (unsigned i = 0; i < N - 1; ++i)
                {
                        if (dimensions[i])
                        {
                                ASSERT(count < result.size());
                                result[count][0] = p;
                                result[count][1] = vectors_[i];
                                ++count;
                        }
                }
        };

        // Смещаться по каждому измерению для перехода к другой вершине.
        // Добавлять к массиву рёбер пары, состоящие из вершины и векторов
        // измерений, по которым не смещались для перехода к этой вершине.
        edges_impl<N - 2>(org_, &dimensions, f);

        ASSERT(count == result.size());

        return result;
}

template <std::size_t N, typename T>
const Vector<N, T>& HyperplaneParallelotope<N, T>::org() const
{
        return org_;
}

template <std::size_t N, typename T>
const Vector<N, T>& HyperplaneParallelotope<N, T>::e(unsigned n) const
{
        ASSERT(n < N - 1);
        return vectors_[n];
}
}
