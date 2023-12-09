/*
Copyright (C) 2017-2023 Topological Manifold

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
#include "parallelotope_aa.h"
#include "parallelotope_edges.h"
#include "parallelotope_length.h"
#include "parallelotope_vertices.h"
#include "shape_overlap.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string>

namespace ns::geometry::spatial
{
template <std::size_t N, typename T>
class Parallelotope final
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        static_assert(N <= 27);

        // Object count after binary division
        static constexpr int DIVISIONS = 1 << N;

        struct Planes final
        {
                Vector<N, T> n;
                T d1;
                T d2;
        };

        std::array<Planes, N> planes_;
        Vector<N, T> org_;
        std::array<Vector<N, T>, N> vectors_;

        void set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors);

        enum class IntersectionType
        {
                FARTHEST,
                NEAREST,
                VOLUME
        };

        template <IntersectionType INTERSECTION_TYPE>
        [[nodiscard]] std::optional<T> intersect_impl(const Ray<N, T>& ray, T max_distance) const;

        template <int INDEX, typename F>
        void binary_division_impl(
                const Vector<N, T>& org,
                Vector<N, T>* d1,
                Vector<N, T>* d2,
                const std::array<Vector<N, T>, N>& half_vectors,
                const Vector<N, T>& middle_d,
                const F& f) const;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N;

        using DataType = T;

        static T intersection_cost();

        Parallelotope() = default;

        Parallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors);
        Parallelotope(const Vector<N, T>& org, const std::array<T, N>& vectors);
        Parallelotope(const Vector<N, T>& min, const Vector<N, T>& max);

        [[nodiscard]] Constraints<N, T, 2 * N, 0> constraints() const;

        [[nodiscard]] bool inside(const Vector<N, T>& p) const;

        [[nodiscard]] std::optional<T> intersect(const Ray<N, T>& ray, T max_distance = Limits<T>::max()) const;

        [[nodiscard]] std::optional<T> intersect_farthest(const Ray<N, T>& ray, T max_distance = Limits<T>::max())
                const;

        [[nodiscard]] std::optional<T> intersect_volume(const Ray<N, T>& ray, T max_distance = Limits<T>::max()) const;

        [[nodiscard]] Vector<N, T> normal(const Vector<N, T>& point) const;

        [[nodiscard]] Vector<N, T> project(const Vector<N, T>& point) const;

        [[nodiscard]] std::array<Parallelotope<N, T>, DIVISIONS> binary_division() const;

        [[nodiscard]] const Vector<N, T>& org() const;
        [[nodiscard]] const std::array<Vector<N, T>, N>& vectors() const;

        [[nodiscard]] auto overlap_function() const;

        [[nodiscard]] decltype(auto) edges() const
        {
                return parallelotope_edges(org_, vectors_);
        }

        [[nodiscard]] decltype(auto) length() const
        {
                return parallelotope_length(vectors_);
        }

        [[nodiscard]] decltype(auto) vertices() const
        {
                return parallelotope_vertices(org_, vectors_);
        }

        [[nodiscard]] friend std::string to_string(const Parallelotope<N, T>& p)
        {
                std::string s = "org = " + to_string(p.org()) + "\n";
                const std::array<Vector<N, T>, N>& vectors = p.vectors();
                for (std::size_t i = 0; i < N; ++i)
                {
                        s += "vector[" + to_string(i) + "] = " + to_string(vectors[i]) + ((i < N - 1) ? "\n" : "");
                }
                return s;
        }
};

template <std::size_t N, typename T>
Parallelotope<N, T>::Parallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors)
{
        set_data(org, vectors);
}

template <std::size_t N, typename T>
Parallelotope<N, T>::Parallelotope(const Vector<N, T>& org, const std::array<T, N>& vectors)
{
        std::array<Vector<N, T>, N> v;
        for (std::size_t i = 0; i < N; ++i)
        {
                for (std::size_t j = 0; j < N; ++j)
                {
                        v[i][j] = 0;
                }
                v[i][i] = vectors[i];
        }
        set_data(org, v);
}

template <std::size_t N, typename T>
Parallelotope<N, T>::Parallelotope(const Vector<N, T>& min, const Vector<N, T>& max)
{
        std::array<Vector<N, T>, N> vectors;
        for (std::size_t i = 0; i < N; ++i)
        {
                vectors[i] = Vector<N, T>(0);
                vectors[i][i] = max[i] - min[i];
        }
        set_data(min, vectors);
}

template <std::size_t N, typename T>
void Parallelotope<N, T>::set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors)
{
        org_ = org;
        vectors_ = vectors;

        // distance from point to plane
        // dot(p - org, normal) = dot(p, normal) - dot(org, normal)
        // d = dot(org, normal)
        // Vector n is directed outward and is for the plane with d2 parameter.
        for (std::size_t i = 0; i < N; ++i)
        {
                planes_[i].n = numerical::orthogonal_complement(del_elem(vectors_, i)).normalized();
                if (dot(planes_[i].n, vectors_[i]) < 0)
                {
                        planes_[i].n = -planes_[i].n;
                }
                planes_[i].d1 = dot(org_, planes_[i].n);
                planes_[i].d2 = dot(org_ + vectors_[i], planes_[i].n);
        }
}

// 2 * N constraints b + a * x >= 0
template <std::size_t N, typename T>
Constraints<N, T, 2 * N, 0> Parallelotope<N, T>::constraints() const
{
        Constraints<N, T, 2 * N, 0> res;

        // Planes n * x - d have vectors n directed outward.
        // Points are inside if n * x - d <= 0 or d + -(n * x) >= 0.
        for (std::size_t i = 0, c_i = 0; i < N; ++i, c_i += 2)
        {
                res.c[c_i].a = planes_[i].n;
                res.c[c_i].b = -planes_[i].d1;

                res.c[c_i + 1].a = -planes_[i].n;
                res.c[c_i + 1].b = planes_[i].d2;
        }

        return res;
}

template <std::size_t N, typename T>
template <Parallelotope<N, T>::IntersectionType INTERSECTION_TYPE>
std::optional<T> Parallelotope<N, T>::intersect_impl(const Ray<N, T>& ray, const T max_distance) const
{
        T near = 0;
        T far = max_distance;

        for (std::size_t i = 0; i < N; ++i)
        {
                const T s = dot(ray.dir(), planes_[i].n);
                const T d = dot(ray.org(), planes_[i].n);
                if (s == 0)
                {
                        if (d < planes_[i].d1 || d > planes_[i].d2)
                        {
                                return {};
                        }
                        continue;
                }
                const bool dir_negative = (s < 0);
                const T r = 1 / s;
                const std::array<T, 2> a{(planes_[i].d1 - d) * r, (planes_[i].d2 - d) * r};
                near = std::max(near, a[dir_negative]);
                far = std::min(far, a[!dir_negative]);
                if (far < near)
                {
                        return {};
                }
        }

        static_assert(
                INTERSECTION_TYPE == IntersectionType::FARTHEST || INTERSECTION_TYPE == IntersectionType::NEAREST
                || INTERSECTION_TYPE == IntersectionType::VOLUME);

        switch (INTERSECTION_TYPE)
        {
        case IntersectionType::FARTHEST:
                return far;
        case IntersectionType::NEAREST:
                return near > 0 ? near : far;
        case IntersectionType::VOLUME:
                return near;
        }
}

template <std::size_t N, typename T>
std::optional<T> Parallelotope<N, T>::intersect(const Ray<N, T>& ray, const T max_distance) const
{
        return intersect_impl<IntersectionType::NEAREST>(ray, max_distance);
}

template <std::size_t N, typename T>
std::optional<T> Parallelotope<N, T>::intersect_farthest(const Ray<N, T>& ray, const T max_distance) const
{
        return intersect_impl<IntersectionType::FARTHEST>(ray, max_distance);
}

template <std::size_t N, typename T>
std::optional<T> Parallelotope<N, T>::intersect_volume(const Ray<N, T>& ray, const T max_distance) const
{
        return intersect_impl<IntersectionType::VOLUME>(ray, max_distance);
}

template <std::size_t N, typename T>
Vector<N, T> Parallelotope<N, T>::normal(const Vector<N, T>& point) const
{
        // the normal of the plane closest to the point

        T min_distance = Limits<T>::max();

        Vector<N, T> n;
        for (std::size_t i = 0; i < N; ++i)
        {
                const T d = dot(point, planes_[i].n);

                {
                        const T distance = std::abs(d - planes_[i].d1);
                        if (distance < min_distance)
                        {
                                min_distance = distance;
                                n = -planes_[i].n;
                        }
                }

                {
                        const T distance = std::abs(d - planes_[i].d2);
                        if (distance < min_distance)
                        {
                                min_distance = distance;
                                n = planes_[i].n;
                        }
                }
        }

        ASSERT(min_distance < Limits<T>::max());

        return n;
}

template <std::size_t N, typename T>
Vector<N, T> Parallelotope<N, T>::project(const Vector<N, T>& point) const
{
        T min_distance = Limits<T>::max();

        unsigned plane_index = Limits<unsigned>::max();
        T plane_distance = Limits<T>::max();
        for (std::size_t i = 0; i < N; ++i)
        {
                const T d = dot(point, planes_[i].n);

                {
                        const T distance = d - planes_[i].d1;
                        const T abs_distance = std::abs(distance);
                        if (abs_distance < min_distance)
                        {
                                min_distance = abs_distance;
                                plane_index = i;
                                plane_distance = distance;
                        }
                }

                {
                        const T distance = d - planes_[i].d2;
                        const T abs_distance = std::abs(distance);
                        if (abs_distance < min_distance)
                        {
                                min_distance = abs_distance;
                                plane_index = i;
                                plane_distance = distance;
                        }
                }
        }

        ASSERT(min_distance < Limits<T>::max());

        return point - planes_[plane_index].n * plane_distance;
}

template <std::size_t N, typename T>
bool Parallelotope<N, T>::inside(const Vector<N, T>& point) const
{
        for (std::size_t i = 0; i < N; ++i)
        {
                const T d = dot(point, planes_[i].n);

                if (d < planes_[i].d1)
                {
                        return false;
                }

                if (d > planes_[i].d2)
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, typename T>
template <int INDEX, typename F>
void Parallelotope<N, T>::binary_division_impl(
        const Vector<N, T>& org,
        Vector<N, T>* const d1,
        Vector<N, T>* const d2,
        const std::array<Vector<N, T>, N>& half_vectors,
        const Vector<N, T>& middle_d,
        const F& f) const
{
        if constexpr (INDEX >= 0)
        {
                (*d1)[INDEX] = planes_[INDEX].d1;
                (*d2)[INDEX] = middle_d[INDEX];
                binary_division_impl<INDEX - 1>(org, d1, d2, half_vectors, middle_d, f);
                (*d1)[INDEX] = middle_d[INDEX];
                (*d2)[INDEX] = planes_[INDEX].d2;
                binary_division_impl<INDEX - 1>(org + half_vectors[INDEX], d1, d2, half_vectors, middle_d, f);
        }
        else
        {
                f(org);
        }
}

template <std::size_t N, typename T>
std::array<Parallelotope<N, T>, Parallelotope<N, T>::DIVISIONS> Parallelotope<N, T>::binary_division() const
{
        std::array<Parallelotope, DIVISIONS> res;

        std::array<Vector<N, T>, N> half_vectors;
        Vector<N, T> middle_d;
        for (std::size_t i = 0; i < N; ++i)
        {
                half_vectors[i] = vectors_[i] / static_cast<T>(2);
                middle_d[i] = (planes_[i].d2 + planes_[i].d1) / static_cast<T>(2);
        }

        for (Parallelotope& p : res)
        {
                p.vectors_ = half_vectors;
                for (std::size_t i = 0; i < N; ++i)
                {
                        p.planes_[i].n = planes_[i].n;
                }
        }

        Vector<N, T> d1;
        Vector<N, T> d2;
        unsigned count = 0;

        const auto f = [&count, &res, &d1, &d2](const Vector<N, T>& org)
        {
                ASSERT(count < res.size());
                res[count].org_ = org;
                for (std::size_t i = 0; i < N; ++i)
                {
                        res[count].planes_[i].d1 = d1[i];
                        res[count].planes_[i].d2 = d2[i];
                }
                ++count;
        };

        binary_division_impl<N - 1>(org_, &d1, &d2, half_vectors, middle_d, f);

        ASSERT(count == res.size());

        return res;
}

template <std::size_t N, typename T>
const Vector<N, T>& Parallelotope<N, T>::org() const
{
        return org_;
}

template <std::size_t N, typename T>
const std::array<Vector<N, T>, N>& Parallelotope<N, T>::vectors() const
{
        return vectors_;
}

template <std::size_t N, typename T>
auto Parallelotope<N, T>::overlap_function() const
{
        return [s = ShapeOverlap(this)](const ShapeOverlap<ParallelotopeAA<N, T>>& p)
        {
                return shapes_overlap(s, p);
        };
}
}
