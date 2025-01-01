/*
Copyright (C) 2017-2025 Topological Manifold

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
#include "parallelotope_edges.h"
#include "parallelotope_length.h"
#include "shape_overlap.h"

#include <src/com/error.h>
#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <utility>

namespace ns::geometry::spatial
{
template <std::size_t N, typename T>
class HyperplaneParallelotope final
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        static_assert(N <= 30);

        static constexpr int VERTEX_COUNT = 1 << (N - 1);

        Hyperplane<N, T> plane_;
        std::array<Hyperplane<N, T>, N - 1> planes_;

        numerical::Vector<N, T> org_;
        std::array<numerical::Vector<N, T>, N - 1> vectors_;

        template <int INDEX, typename F>
        void vertices_impl(const numerical::Vector<N, T>& p, const F& f) const;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N - 1;

        using DataType = T;

        static T intersection_cost();

        HyperplaneParallelotope(
                const numerical::Vector<N, T>& org,
                const std::array<numerical::Vector<N, T>, N - 1>& vectors);

        void set_normal_direction(const numerical::Vector<N, T>& direction);

        [[nodiscard]] Constraints<N, T, 2 * (N - 1), 1> constraints() const;

        [[nodiscard]] std::optional<T> intersect(const numerical::Ray<N, T>& r) const;

        [[nodiscard]] const numerical::Vector<N, T>& normal() const;

        [[nodiscard]] numerical::Vector<N, T> project(const numerical::Vector<N, T>& point) const;

        [[nodiscard]] std::array<numerical::Vector<N, T>, VERTEX_COUNT> vertices() const;

        [[nodiscard]] decltype(auto) edges() const
        {
                return parallelotope_edges(org_, vectors_);
        }

        [[nodiscard]] decltype(auto) length() const
        {
                return parallelotope_length(vectors_);
        }

        [[nodiscard]] const numerical::Vector<N, T>& org() const;
        [[nodiscard]] const std::array<numerical::Vector<N, T>, N - 1>& vectors() const;

        [[nodiscard]] auto overlap_function() const;
};

template <std::size_t N, typename T>
HyperplaneParallelotope<N, T>::HyperplaneParallelotope(
        const numerical::Vector<N, T>& org,
        const std::array<numerical::Vector<N, T>, N - 1>& vectors)
        : org_(org),
          vectors_(vectors)
{
        plane_.n = numerical::orthogonal_complement(vectors).normalized();
        if (!is_finite(plane_.n))
        {
                error("Hyperplane parallelotope normal " + to_string(plane_.n) + " is not finite, vectors "
                      + to_string(vectors));
        }
        plane_.d = dot(plane_.n, org_);

        for (std::size_t i = 0; i < N - 1; ++i)
        {
                std::swap(plane_.n, vectors_[i]);
                planes_[i].n = numerical::orthogonal_complement(vectors_);
                std::swap(plane_.n, vectors_[i]);

                if (dot(planes_[i].n, vectors_[i]) < 0)
                {
                        planes_[i].n = -planes_[i].n;
                }

                planes_[i].d = dot(org_, planes_[i].n);

                const T distance = planes_[i].distance(org_ + vectors_[i]);
                ASSERT(distance >= 0);
                planes_[i].n /= distance;
                planes_[i].d /= distance;
        }
}

template <std::size_t N, typename T>
void HyperplaneParallelotope<N, T>::set_normal_direction(const numerical::Vector<N, T>& direction)
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
        Constraints<N, T, 2 * (N - 1), 1> res;

        // Planes n * x - d have vectors n directed outward.
        // Points are inside if n * x - d <= 0 or d + -(n * x) >= 0.
        for (std::size_t i = 0, c_i = 0; i < N - 1; ++i, c_i += 2)
        {
                const T len = planes_[i].n.norm();

                res.c[c_i].a = planes_[i].n / len;
                res.c[c_i].b = -planes_[i].d / len;

                res.c[c_i + 1].a = -planes_[i].n / len;
                res.c[c_i + 1].b = dot(org_ + vectors_[i], planes_[i].n) / len;
        }

        res.c_eq[0].a = plane_.n;
        res.c_eq[0].b = -plane_.d;

        return res;
}

template <std::size_t N, typename T>
std::optional<T> HyperplaneParallelotope<N, T>::intersect(const numerical::Ray<N, T>& ray) const
{
        const T t = plane_.intersect(ray);
        if (!(t > 0))
        {
                return std::nullopt;
        }

        const numerical::Vector<N, T> point = ray.point(t);

        for (std::size_t i = 0; i < N - 1; ++i)
        {
                const T d = planes_[i].distance(point);
                if (!(d > 0 && d < 1))
                {
                        return std::nullopt;
                }
        }
        return t;
}

template <std::size_t N, typename T>
const numerical::Vector<N, T>& HyperplaneParallelotope<N, T>::normal() const
{
        return plane_.n;
}

template <std::size_t N, typename T>
numerical::Vector<N, T> HyperplaneParallelotope<N, T>::project(const numerical::Vector<N, T>& point) const
{
        return plane_.project(point);
}

template <std::size_t N, typename T>
template <int INDEX, typename F>
void HyperplaneParallelotope<N, T>::vertices_impl(const numerical::Vector<N, T>& p, const F& f) const
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
std::array<numerical::Vector<N, T>, HyperplaneParallelotope<N, T>::VERTEX_COUNT> HyperplaneParallelotope<N, T>::
        vertices() const
{
        std::array<numerical::Vector<N, T>, VERTEX_COUNT> res;

        unsigned count = 0;

        const auto f = [&count, &res](const numerical::Vector<N, T>& p)
        {
                ASSERT(count < res.size());
                res[count++] = p;
        };

        vertices_impl<N - 2>(org_, f);

        ASSERT(count == res.size());

        return res;
}

template <std::size_t N, typename T>
const numerical::Vector<N, T>& HyperplaneParallelotope<N, T>::org() const
{
        return org_;
}

template <std::size_t N, typename T>
const std::array<numerical::Vector<N, T>, N - 1>& HyperplaneParallelotope<N, T>::vectors() const
{
        return vectors_;
}

template <std::size_t N, typename T>
auto HyperplaneParallelotope<N, T>::overlap_function() const
{
        return [s = ShapeOverlap(this)](const ShapeOverlap<ParallelotopeAA<N, T>>& p)
        {
                return shapes_overlap(s, p);
        };
}
}
