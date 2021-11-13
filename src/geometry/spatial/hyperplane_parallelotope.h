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
#include "parallelotope_aa.h"
#include "shape_overlap.h"

#include <src/com/error.h>
#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <utility>

namespace ns::geometry
{
template <std::size_t N, typename T>
class HyperplaneParallelotope final
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        static_assert(N <= 30);

        static constexpr int DIAGONAL_COUNT = 1 << (N - 2);
        static constexpr int VERTEX_COUNT = 1 << (N - 1);

        // vertex count 2 ^ (N - 1) multiplied by vertex dimension
        // count (N - 1) and divided by 2 for uniqueness
        // ((2 ^ (N - 1)) * (N - 1)) / 2 = (2 ^ (N - 2)) * (N - 1)
        static constexpr int EDGE_COUNT = (1 << (N - 2)) * (N - 1);

        Hyperplane<N, T> plane_;
        std::array<Hyperplane<N, T>, N - 1> planes_;

        Vector<N, T> org_;
        std::array<Vector<N, T>, N - 1> vectors_;

        template <int INDEX, typename F>
        void vertices_impl(const Vector<N, T>& p, const F& f) const;

        template <int INDEX, typename F>
        void edges_impl(const Vector<N, T>& p, std::array<bool, N - 1>* dimensions, const F& f) const;

        template <int INDEX, typename F>
        void length_impl(const Vector<N, T>& sum, const F& f) const;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N - 1;

        using DataType = T;

        template <typename... P>
        explicit HyperplaneParallelotope(const Vector<N, T>& org, const P&... vectors);

        HyperplaneParallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors);

        void set_normal_direction(const Vector<N, T>& direction);

        Constraints<N, T, 2 * (N - 1), 1> constraints() const;

        std::optional<T> intersect(const Ray<N, T>& r) const;

        const Vector<N, T>& normal() const;

        std::array<Vector<N, T>, VERTEX_COUNT> vertices() const;

        std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> edges() const;

        T length() const;

        const Vector<N, T>& org() const;
        const std::array<Vector<N, T>, N - 1>& vectors() const;

        auto overlap_function() const;
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
        : org_(org), vectors_(vectors)
{
        plane_.n = numerical::orthogonal_complement(vectors).normalized();
        if (!is_finite(plane_.n))
        {
                error("Hyperplane parallelotope normal " + to_string(plane_.n) + " is not finite, vectors "
                      + to_string(vectors));
        }
        plane_.d = dot(plane_.n, org_);

        for (unsigned i = 0; i < N - 1; ++i)
        {
                std::swap(plane_.n, vectors_[i]);
                planes_[i].n = numerical::orthogonal_complement(vectors_);
                std::swap(plane_.n, vectors_[i]);

                if (dot(planes_[i].n, vectors_[i]) < 0)
                {
                        planes_[i].n = -planes_[i].n;
                }

                planes_[i].d = dot(org_, planes_[i].n);

                const T distance = dot(org_ + vectors_[i], planes_[i].n) - planes_[i].d;
                ASSERT(distance >= 0);
                planes_[i].n /= distance;
                planes_[i].d /= distance;
        }
}

template <std::size_t N, typename T>
void HyperplaneParallelotope<N, T>::set_normal_direction(const Vector<N, T>& direction)
{
        if (dot(plane_.n, direction) < 0)
        {
                plane_.reverse_normal();
        }
}

// 2 * (N - 1) constraints b + a * x >= 0
// one constraint b + a * x = 0
template <std::size_t N, typename T>
Constraints<N, T, 2 * (N - 1), 1> HyperplaneParallelotope<N, T>::constraints() const
{
        Constraints<N, T, 2 * (N - 1), 1> result;

        // Planes n * x - d have vectors n directed outward.
        // Points are inside if n * x - d <= 0 or d + -(n * x) >= 0.
        for (unsigned i = 0, c_i = 0; i < N - 1; ++i, c_i += 2)
        {
                const T len = planes_[i].n.norm();

                result.c[c_i].a = planes_[i].n / len;
                result.c[c_i].b = -planes_[i].d / len;

                result.c[c_i + 1].a = -planes_[i].n / len;
                result.c[c_i + 1].b = dot(org_ + vectors_[i], planes_[i].n) / len;
        }

        result.c_eq[0].a = plane_.n;
        result.c_eq[0].b = -plane_.d;

        return result;
}

template <std::size_t N, typename T>
std::optional<T> HyperplaneParallelotope<N, T>::intersect(const Ray<N, T>& ray) const
{
        const T t = plane_.intersect(ray);
        if (!(t > 0))
        {
                return std::nullopt;
        }

        const Vector<N, T> point = ray.point(t);

        for (unsigned i = 0; i < N - 1; ++i)
        {
                const T d = dot(point, planes_[i].n) - planes_[i].d;
                if (!(d > 0 && d < 1))
                {
                        return std::nullopt;
                }
        }
        return t;
}

template <std::size_t N, typename T>
const Vector<N, T>& HyperplaneParallelotope<N, T>::normal() const
{
        return plane_.n;
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
void HyperplaneParallelotope<N, T>::edges_impl(
        const Vector<N, T>& p,
        std::array<bool, N - 1>* const dimensions,
        const F& f) const
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

        edges_impl<N - 2>(org_, &dimensions, f);

        ASSERT(count == result.size());

        return result;
}

template <std::size_t N, typename T>
template <int INDEX, typename F>
void HyperplaneParallelotope<N, T>::length_impl(const Vector<N, T>& sum, const F& f) const
{
        if constexpr (INDEX >= 0)
        {
                length_impl<INDEX - 1>(sum + vectors_[INDEX], f);
                length_impl<INDEX - 1>(sum - vectors_[INDEX], f);
        }
        else
        {
                f(sum);
        }
}

template <std::size_t N, typename T>
T HyperplaneParallelotope<N, T>::length() const
{
        T max_squared = Limits<T>::lowest();

        unsigned count = 0;

        auto f = [&max_squared, &count](const Vector<N, T>& d)
        {
                ++count;
                max_squared = std::max(max_squared, d.norm_squared());
        };

        // compute all diagonals and find the diagonal with the maximum length
        constexpr int LAST_INDEX = N - 2;
        length_impl<LAST_INDEX - 1>(vectors_[LAST_INDEX], f);

        ASSERT(count == DIAGONAL_COUNT);

        return std::sqrt(max_squared);
}

template <std::size_t N, typename T>
const Vector<N, T>& HyperplaneParallelotope<N, T>::org() const
{
        return org_;
}

template <std::size_t N, typename T>
const std::array<Vector<N, T>, N - 1>& HyperplaneParallelotope<N, T>::vectors() const
{
        return vectors_;
}

template <std::size_t N, typename T>
auto HyperplaneParallelotope<N, T>::overlap_function() const
{
        return [s = geometry::ShapeOverlap(this)](const geometry::ShapeOverlap<geometry::ParallelotopeAA<N, T>>& p)
        {
                return geometry::shapes_overlap(s, p);
        };
}
}
