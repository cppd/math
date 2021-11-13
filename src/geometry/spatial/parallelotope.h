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

/*
Samuel R. Buss.
3D Computer Graphics. A Mathematical Introduction with OpenGL.
Cambridge University Press, 2003.
*/

#pragma once

#include "constraint.h"
#include "parallelotope_aa.h"
#include "shape_overlap.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/numerical/complement.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace ns::geometry
{
namespace parallelotope_implementation
{
template <std::size_t N, typename T, std::size_t... I>
std::array<Vector<N, T>, N> make_vectors_impl(const Vector<N, T>& d, std::integer_sequence<std::size_t, I...>)
{
        static_assert(N == sizeof...(I));
        std::array<Vector<N, T>, N> vectors{(static_cast<void>(I), Vector<N, T>(0))...};
        ((vectors[I][I] = d[I]), ...);
        return vectors;
}
// Diagonal matrix NxN
template <std::size_t N, typename T>
std::array<Vector<N, T>, N> make_vectors(const Vector<N, T>& d)
{
        return make_vectors_impl(d, std::make_integer_sequence<std::size_t, N>());
}
}

template <std::size_t N, typename T>
class Parallelotope final
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        static_assert(N <= 27);

        // Object count after binary division
        static constexpr int DIVISIONS = 1 << N;

        static constexpr int DIAGONAL_COUNT = 1 << (N - 1);
        static constexpr int VERTEX_COUNT = 1 << N;

        // Vertex count 2 ^ N multiplied by vertex dimension
        // count N and divided by 2 for uniqueness
        // ((2 ^ N) * N) / 2 = (2 ^ (N - 1)) * N
        static constexpr int EDGE_COUNT = (1 << (N - 1)) * N;

        struct Planes
        {
                Vector<N, T> n;
                T d1, d2;
        };
        std::array<Planes, N> planes_;

        Vector<N, T> org_;
        std::array<Vector<N, T>, N> vectors_;

        void set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors);

        bool intersect_impl(const Ray<N, T>& r, T* first, T* second) const;

        template <int INDEX, typename F>
        void binary_division_impl(
                const Vector<N, T>& org,
                Vector<N, T>* d1,
                Vector<N, T>* d2,
                const std::array<Vector<N, T>, N>& half_vectors,
                const Vector<N, T>& middle_d,
                const F& f) const;

        template <int INDEX, typename F>
        void vertices_impl(const Vector<N, T>& p, const F& f) const;

        template <int INDEX, typename F>
        void edges_impl(const Vector<N, T>& p, std::array<bool, N>* dimensions, const F& f) const;

        template <int INDEX, typename F>
        void length_impl(const Vector<N, T>& sum, const F& f) const;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N;

        using DataType = T;

        static T intersection_cost();

        Parallelotope() = default;

        template <typename... P>
        explicit Parallelotope(const Vector<N, T>& org, const P&... vectors);
        Parallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors);
        Parallelotope(const Vector<N, T>& min, const Vector<N, T>& max);

        Constraints<N, T, 2 * N, 0> constraints() const;

        bool inside(const Vector<N, T>& p) const;

        std::optional<T> intersect(const Ray<N, T>& r) const;
        std::optional<T> intersect_farthest(const Ray<N, T>& r) const;
        std::optional<T> intersect_volume(const Ray<N, T>& r) const;

        Vector<N, T> normal(const Vector<N, T>& p) const;

        std::array<Parallelotope<N, T>, DIVISIONS> binary_division() const;

        std::array<Vector<N, T>, VERTEX_COUNT> vertices() const;

        std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> edges() const;

        T length() const;

        const Vector<N, T>& org() const;
        const std::array<Vector<N, T>, N>& vectors() const;

        auto overlap_function() const;
};

template <std::size_t N, typename T>
template <typename... P>
Parallelotope<N, T>::Parallelotope(const Vector<N, T>& org, const P&... vectors)
{
        static_assert((std::is_same_v<Vector<N, T>, P> && ...));
        static_assert(sizeof...(P) == N);

        set_data(org, {vectors...});
}

template <std::size_t N, typename T>
Parallelotope<N, T>::Parallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors)
{
        set_data(org, vectors);
}

template <std::size_t N, typename T>
Parallelotope<N, T>::Parallelotope(const Vector<N, T>& min, const Vector<N, T>& max)
{
        set_data(min, parallelotope_implementation::make_vectors(max - min));
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
        for (unsigned i = 0; i < N; ++i)
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
        Constraints<N, T, 2 * N, 0> result;

        // Planes n * x - d have vectors n directed outward.
        // Points are inside if n * x - d <= 0 or d + -(n * x) >= 0.
        for (unsigned i = 0, c_i = 0; i < N; ++i, c_i += 2)
        {
                result.c[c_i].a = planes_[i].n;
                result.c[c_i].b = -planes_[i].d1;

                result.c[c_i + 1].a = -planes_[i].n;
                result.c[c_i + 1].b = planes_[i].d2;
        }

        return result;
}

template <std::size_t N, typename T>
bool Parallelotope<N, T>::intersect_impl(const Ray<N, T>& r, T* first, T* second) const
{
        T near = 0;
        T far = Limits<T>::max();

        for (unsigned i = 0; i < N; ++i)
        {
                const T s = dot(r.dir(), planes_[i].n);
                if (s == 0)
                {
                        // parallel to the planes
                        T d = dot(r.org(), planes_[i].n);
                        if (d < planes_[i].d1 || d > planes_[i].d2)
                        {
                                // outside the planes
                                return false;
                        }
                        // inside the planes
                        continue;
                }

                const T d = dot(r.org(), planes_[i].n);
                const T alpha1 = (planes_[i].d1 - d) / s;
                const T alpha2 = (planes_[i].d2 - d) / s;

                if (s > 0)
                {
                        // front intersection for the first plane
                        // back intersection for the second plane
                        near = std::max(alpha1, near);
                        far = std::min(alpha2, far);
                }
                else
                {
                        // front intersection for the second plane
                        // back intersection for the first plane
                        near = std::max(alpha2, near);
                        far = std::min(alpha1, far);
                }

                if (far < near)
                {
                        return false;
                }
        }

        *first = near;
        *second = far;

        return true;
}

template <std::size_t N, typename T>
std::optional<T> Parallelotope<N, T>::intersect(const Ray<N, T>& r) const
{
        T first;
        T second;
        if (intersect_impl(r, &first, &second))
        {
                return (first > 0) ? first : second;
        }
        return std::nullopt;
}

template <std::size_t N, typename T>
std::optional<T> Parallelotope<N, T>::intersect_farthest(const Ray<N, T>& r) const
{
        T first;
        T second;
        if (intersect_impl(r, &first, &second))
        {
                return second;
        }
        return std::nullopt;
}

template <std::size_t N, typename T>
std::optional<T> Parallelotope<N, T>::intersect_volume(const Ray<N, T>& r) const
{
        T first;
        T second;
        if (intersect_impl(r, &first, &second))
        {
                return first;
        }
        return std::nullopt;
}

template <std::size_t N, typename T>
Vector<N, T> Parallelotope<N, T>::normal(const Vector<N, T>& p) const
{
        // the normal of the plane closest to the point

        T min_distance = Limits<T>::max();

        Vector<N, T> n;
        for (unsigned i = 0; i < N; ++i)
        {
                const T d = dot(p, planes_[i].n);

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
bool Parallelotope<N, T>::inside(const Vector<N, T>& p) const
{
        for (unsigned i = 0; i < N; ++i)
        {
                const T d = dot(p, planes_[i].n);

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
        Vector<N, T>* d1,
        Vector<N, T>* d2,
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
        std::array<Parallelotope, DIVISIONS> result;

        std::array<Vector<N, T>, N> half_vectors;
        Vector<N, T> middle_d;
        for (unsigned i = 0; i < N; ++i)
        {
                half_vectors[i] = vectors_[i] / static_cast<T>(2);
                middle_d[i] = (planes_[i].d2 + planes_[i].d1) / static_cast<T>(2);
        }

        for (Parallelotope& p : result)
        {
                p.vectors_ = half_vectors;
                for (unsigned i = 0; i < N; ++i)
                {
                        p.planes_[i].n = planes_[i].n;
                }
        }

        Vector<N, T> d1;
        Vector<N, T> d2;
        unsigned count = 0;

        const auto f = [&count, &result, &d1, &d2](const Vector<N, T>& org)
        {
                ASSERT(count < result.size());
                result[count].org_ = org;
                for (unsigned i = 0; i < N; ++i)
                {
                        result[count].planes_[i].d1 = d1[i];
                        result[count].planes_[i].d2 = d2[i];
                }
                ++count;
        };

        binary_division_impl<N - 1>(org_, &d1, &d2, half_vectors, middle_d, f);

        ASSERT(count == result.size());

        return result;
}

template <std::size_t N, typename T>
template <int INDEX, typename F>
void Parallelotope<N, T>::vertices_impl(const Vector<N, T>& p, const F& f) const
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
std::array<Vector<N, T>, Parallelotope<N, T>::VERTEX_COUNT> Parallelotope<N, T>::vertices() const
{
        std::array<Vector<N, T>, VERTEX_COUNT> result;

        unsigned count = 0;

        const auto f = [&count, &result](const Vector<N, T>& p)
        {
                ASSERT(count < result.size());
                result[count++] = p;
        };

        vertices_impl<N - 1>(org_, f);

        ASSERT(count == result.size());

        return result;
}

template <std::size_t N, typename T>
template <int INDEX, typename F>
void Parallelotope<N, T>::edges_impl(const Vector<N, T>& p, std::array<bool, N>* dimensions, const F& f) const
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
std::array<std::array<Vector<N, T>, 2>, Parallelotope<N, T>::EDGE_COUNT> Parallelotope<N, T>::edges() const
{
        static_assert(N <= 3);

        std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> result;

        unsigned count = 0;
        std::array<bool, N> dimensions;

        const auto f = [this, &dimensions, &count, &result](const Vector<N, T>& p)
        {
                for (unsigned i = 0; i < N; ++i)
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

        edges_impl<N - 1>(org_, &dimensions, f);

        ASSERT(count == result.size());

        return result;
}

template <std::size_t N, typename T>
template <int INDEX, typename F>
void Parallelotope<N, T>::length_impl(const Vector<N, T>& sum, const F& f) const
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
T Parallelotope<N, T>::length() const
{
        T max_squared = Limits<T>::lowest();

        unsigned count = 0;

        const auto f = [&max_squared, &count](const Vector<N, T>& d)
        {
                ++count;
                max_squared = std::max(max_squared, d.norm_squared());
        };

        // compute all diagonals and find the diagonal with the maximum length
        constexpr int LAST_INDEX = N - 1;
        length_impl<LAST_INDEX - 1>(vectors_[LAST_INDEX], f);

        ASSERT(count == DIAGONAL_COUNT);

        return std::sqrt(max_squared);
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

namespace ns
{
template <std::size_t N, typename T>
std::string to_string(const geometry::Parallelotope<N, T>& p)
{
        std::string s;
        s += "org = " + to_string(p.org()) + "\n";

        const std::array<Vector<N, T>, N>& vectors = p.vectors();
        for (unsigned i = 0; i < N; ++i)
        {
                s += "edge[" + to_string(i) + "] = " + to_string(vectors[i]) + ((i < N - 1) ? "\n" : "");
        }
        return s;
}
}
