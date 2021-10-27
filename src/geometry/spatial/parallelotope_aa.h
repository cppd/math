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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace ns::geometry
{
namespace parallelotope_aa_implementation
{
template <std::size_t N, typename T, std::size_t... I>
constexpr std::array<Vector<N, T>, N> make_vectors_impl(const T& v, std::integer_sequence<std::size_t, I...>)
{
        static_assert(N == sizeof...(I));
        std::array<Vector<N, T>, N> vectors{(static_cast<void>(I), Vector<N, T>(0))...};
        ((vectors[I][I] = v), ...);
        return vectors;
}
// Diagonal matrix NxN
template <std::size_t N, typename T, std::size_t... I>
constexpr std::array<Vector<N, T>, N> make_vectors(const T& v)
{
        return make_vectors_impl<N, T>(v, std::make_integer_sequence<std::size_t, N>());
}

template <std::size_t N, typename T>
constexpr Vector<N, T> index_vector(unsigned index, T value)
{
        Vector<N, T> v(0);
        v[index] = value;
        return v;
}
}

template <std::size_t N, typename T>
class ParallelotopeAA final
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        // Example: {(1, 0, 0), (0, 1, 0), (0, 0, 1)}
        static constexpr std::array<Vector<N, T>, N> NORMALS_POSITIVE =
                parallelotope_aa_implementation::make_vectors<N, T>(1);
        // Example: {(-1, 0, 0), (0, -1, 0), (0, 0, -1)}
        static constexpr std::array<Vector<N, T>, N> NORMALS_NEGATIVE =
                parallelotope_aa_implementation::make_vectors<N, T>(-1);

        static_assert(N <= 27);

        // Object count after binary division
        static constexpr int DIVISIONS = 1 << N;

        static constexpr int VERTEX_COUNT = 1 << N;

        // Vertex count 2 ^ N multiplied by vertex dimension
        // count N and divided by 2 for uniqueness
        // ((2 ^ N) * N) / 2 = (2 ^ (N - 1)) * N
        static constexpr int EDGE_COUNT = (1 << (N - 1)) * N;

        struct Planes
        {
                T d1, d2;
        };
        std::array<Planes, N> planes_;

        void set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N>& vectors);
        void set_data(const Vector<N, T>& org, const std::array<T, N>& sizes);

        bool intersect_impl(const Ray<N, T>& r, T* first, T* second) const;

        T size(unsigned i) const;

        template <int INDEX, typename F>
        void binary_division_impl(std::array<Planes, N>* p, const Vector<N, T>& middle_d, const F& f) const;

        template <int INDEX, typename F>
        void vertices_impl(Vector<N, T>* p, const F& f) const;

public:
        static constexpr std::size_t SPACE_DIMENSION = N;
        static constexpr std::size_t SHAPE_DIMENSION = N;

        using DataType = T;

        ParallelotopeAA() = default;

        template <typename... P>
        explicit ParallelotopeAA(const Vector<N, T>& org, const P&... sizes);
        ParallelotopeAA(const Vector<N, T>& org, const std::array<T, N>& sizes);
        ParallelotopeAA(const Vector<N, T>& min, const Vector<N, T>& max);

        Constraints<N, T, 2 * N, 0> constraints() const;

        bool inside(const Vector<N, T>& p) const;

        std::optional<T> intersect(const Ray<N, T>& r) const;
        std::optional<T> intersect_farthest(const Ray<N, T>& r) const;
        std::optional<T> intersect_volume(const Ray<N, T>& r) const;

        Vector<N, T> normal(const Vector<N, T>& p) const;

        std::array<ParallelotopeAA<N, T>, DIVISIONS> binary_division() const;

        std::array<Vector<N, T>, VERTEX_COUNT> vertices() const;

        std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> edges() const;

        T length() const;

        Vector<N, T> org() const;
        std::array<Vector<N, T>, N> vectors() const;

        Vector<N, T> min() const;
        Vector<N, T> max() const;
};

template <std::size_t N, typename T>
template <typename... P>
ParallelotopeAA<N, T>::ParallelotopeAA(const Vector<N, T>& org, const P&... sizes)
        : ParallelotopeAA(org, std::array<T, N>{sizes...})
{
        static_assert((std::is_same_v<T, P> && ...));
        static_assert(sizeof...(P) == N);
}

template <std::size_t N, typename T>
ParallelotopeAA<N, T>::ParallelotopeAA(const Vector<N, T>& org, const std::array<T, N>& sizes)
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (!(sizes[i] > 0))
                {
                        error("Axis-aligned parallelotope sizes " + to_string(sizes));
                }
        }
        for (unsigned i = 0; i < N; ++i)
        {
                planes_[i].d1 = org[i];
                planes_[i].d2 = org[i] + sizes[i];
        }
}

template <std::size_t N, typename T>
ParallelotopeAA<N, T>::ParallelotopeAA(const Vector<N, T>& min, const Vector<N, T>& max)
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (!(max[i] - min[i] > 0))
                {
                        error("Axis-aligned parallelotope min " + to_string(min) + ", max " + to_string(max));
                }
        }
        for (unsigned i = 0; i < N; ++i)
        {
                planes_[i].d1 = min[i];
                planes_[i].d2 = max[i];
        }
}

template <std::size_t N, typename T>
T ParallelotopeAA<N, T>::size(unsigned i) const
{
        return planes_[i].d2 - planes_[i].d1;
}

// 2 * N constraints b + a * x >= 0
template <std::size_t N, typename T>
Constraints<N, T, 2 * N, 0> ParallelotopeAA<N, T>::constraints() const
{
        Constraints<N, T, 2 * N, 0> result;

        // Planes n * x - d have vectors n directed outward.
        // Points are inside if n * x - d <= 0 or d + -(n * x) >= 0.
        for (unsigned i = 0, c_i = 0; i < N; ++i, c_i += 2)
        {
                result.c[c_i].a = NORMALS_POSITIVE[i];
                result.c[c_i].b = -planes_[i].d1;

                result.c[c_i + 1].a = NORMALS_NEGATIVE[i];
                result.c[c_i + 1].b = planes_[i].d2;
        }

        return result;
}

template <std::size_t N, typename T>
bool ParallelotopeAA<N, T>::intersect_impl(const Ray<N, T>& r, T* first, T* second) const
{
        T near = 0;
        T far = Limits<T>::max();

        for (unsigned i = 0; i < N; ++i)
        {
                const T s = r.dir()[i];
                if (s == 0)
                {
                        // parallel to the planes
                        T d = r.org()[i];
                        if (d < planes_[i].d1 || d > planes_[i].d2)
                        {
                                // outside the planes
                                return false;
                        }
                        // inside the planes
                        continue;
                }

                const T d = r.org()[i];
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
std::optional<T> ParallelotopeAA<N, T>::intersect(const Ray<N, T>& r) const
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
std::optional<T> ParallelotopeAA<N, T>::intersect_farthest(const Ray<N, T>& r) const
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
std::optional<T> ParallelotopeAA<N, T>::intersect_volume(const Ray<N, T>& r) const
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
Vector<N, T> ParallelotopeAA<N, T>::normal(const Vector<N, T>& p) const
{
        // the normal of the plane closest to the point

        T min = Limits<T>::max();

        Vector<N, T> n;
        for (unsigned i = 0; i < N; ++i)
        {
                T l;

                l = std::abs(p[i] - planes_[i].d1);
                if (l < min)
                {
                        min = l;
                        n = NORMALS_NEGATIVE[i];
                }

                l = std::abs(p[i] - planes_[i].d2);
                if (l < min)
                {
                        min = l;
                        n = NORMALS_POSITIVE[i];
                }
        }

        ASSERT(min < Limits<T>::max());

        return n;
}

template <std::size_t N, typename T>
bool ParallelotopeAA<N, T>::inside(const Vector<N, T>& p) const
{
        for (unsigned i = 0; i < N; ++i)
        {
                if (p[i] < planes_[i].d1)
                {
                        return false;
                }

                if (p[i] > planes_[i].d2)
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t N, typename T>
template <int INDEX, typename F>
void ParallelotopeAA<N, T>::binary_division_impl(std::array<Planes, N>* p, const Vector<N, T>& middle_d, const F& f)
        const
{
        if constexpr (INDEX >= 0)
        {
                (*p)[INDEX].d1 = planes_[INDEX].d1;
                (*p)[INDEX].d2 = middle_d[INDEX];
                binary_division_impl<INDEX - 1>(p, middle_d, f);

                (*p)[INDEX].d1 = middle_d[INDEX];
                (*p)[INDEX].d2 = planes_[INDEX].d2;
                binary_division_impl<INDEX - 1>(p, middle_d, f);
        }
        else
        {
                f();
        }
}

template <std::size_t N, typename T>
std::array<ParallelotopeAA<N, T>, ParallelotopeAA<N, T>::DIVISIONS> ParallelotopeAA<N, T>::binary_division() const
{
        std::array<ParallelotopeAA, DIVISIONS> result;

        Vector<N, T> middle_d;
        for (unsigned i = 0; i < N; ++i)
        {
                middle_d[i] = (planes_[i].d1 + planes_[i].d2) / static_cast<T>(2);
        }

        unsigned count = 0;
        std::array<Planes, N> p;

        const auto f = [&count, &result, &p]()
        {
                ASSERT(count < result.size());
                result[count++].planes_ = p;
        };

        binary_division_impl<N - 1>(&p, middle_d, f);

        ASSERT(count == result.size());

        return result;
}

template <std::size_t N, typename T>
template <int INDEX, typename F>
void ParallelotopeAA<N, T>::vertices_impl(Vector<N, T>* p, const F& f) const
{
        if constexpr (INDEX >= 0)
        {
                (*p)[INDEX] = planes_[INDEX].d1;
                vertices_impl<INDEX - 1>(p, f);
                (*p)[INDEX] = planes_[INDEX].d2;
                vertices_impl<INDEX - 1>(p, f);
        }
        else
        {
                f();
        }
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, ParallelotopeAA<N, T>::VERTEX_COUNT> ParallelotopeAA<N, T>::vertices() const
{
        std::array<Vector<N, T>, VERTEX_COUNT> result;

        unsigned count = 0;
        Vector<N, T> p;

        const auto f = [&count, &result, &p]()
        {
                ASSERT(count < result.size());
                result[count++] = p;
        };

        vertices_impl<N - 1>(&p, f);

        ASSERT(count == result.size());

        return result;
}

template <std::size_t N, typename T>
std::array<std::array<Vector<N, T>, 2>, ParallelotopeAA<N, T>::EDGE_COUNT> ParallelotopeAA<N, T>::edges() const
{
        static_assert(N <= 3);

        std::array<std::array<Vector<N, T>, 2>, EDGE_COUNT> result;

        std::array<Vector<N, T>, N> vectors;
        for (unsigned i = 0; i < N; ++i)
        {
                vectors[i] = parallelotope_aa_implementation::index_vector<N, T>(i, planes_[i].d2 - planes_[i].d1);
        }

        unsigned count = 0;
        Vector<N, T> p;

        const auto f = [this, &count, &result, &p, &vectors]()
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if (p[i] == planes_[i].d1)
                        {
                                ASSERT(count < result.size());
                                result[count][0] = p;
                                result[count][1] = vectors[i];
                                ++count;
                        }
                }
        };

        vertices_impl<N - 1>(&p, f);

        ASSERT(count == result.size());

        return result;
}

template <std::size_t N, typename T>
T ParallelotopeAA<N, T>::length() const
{
        Vector<N, T> s;
        for (unsigned i = 0; i < N; ++i)
        {
                s[i] = planes_[i].d2 - planes_[i].d1;
        }
        return s.norm();
}

template <std::size_t N, typename T>
Vector<N, T> ParallelotopeAA<N, T>::org() const
{
        Vector<N, T> v;
        for (unsigned i = 0; i < N; ++i)
        {
                v[i] = planes_[i].d1;
        }
        return v;
}

template <std::size_t N, typename T>
std::array<Vector<N, T>, N> ParallelotopeAA<N, T>::vectors() const
{
        std::array<Vector<N, T>, N> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = parallelotope_aa_implementation::index_vector<N, T>(i, size(i));
        }
        return res;
}

template <std::size_t N, typename T>
Vector<N, T> ParallelotopeAA<N, T>::min() const
{
        Vector<N, T> v;
        for (unsigned i = 0; i < N; ++i)
        {
                v[i] = planes_[i].d1;
        }
        return v;
}

template <std::size_t N, typename T>
Vector<N, T> ParallelotopeAA<N, T>::max() const
{
        Vector<N, T> v;
        for (unsigned i = 0; i < N; ++i)
        {
                v[i] = planes_[i].d2;
        }
        return v;
}
}

namespace ns
{
template <std::size_t N, typename T>
std::string to_string(const geometry::ParallelotopeAA<N, T>& p)
{
        std::string s;
        s += "org = " + to_string(p.org()) + "\n";

        const std::array<Vector<N, T>, N> vectors = p.vectors();
        for (unsigned i = 0; i < N; ++i)
        {
                s += "edge[" + to_string(i) + "] = " + to_string(vectors[i]) + ((i < N - 1) ? "\n" : "");
        }
        return s;
}
}
