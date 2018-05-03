/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "com/ray.h"
#include "com/vec.h"
#include "geometry/core/linear_algebra.h"
#include "path_tracing/space/simplex_geometry.h"

template <size_t N, typename T>
class Rectangle
{
        static constexpr int VERTEX_COUNT = 1 << (N - 1);

        Vector<N, T> m_org;
        std::array<Vector<N, T>, N - 1> m_vectors;
        Vector<N, T> m_normal;
        ParallelotopeGeometry<N, T> m_geometry;

        void set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors);

public:
        static constexpr size_t DIMENSION = N;
        using DataType = T;

        Rectangle() = default;

        template <typename... P>
        Rectangle(const Vector<N, T>& org, const P&... vectors);

        Rectangle(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors);

        bool intersect(const Ray<N, T>& r, T* t) const;

        Vector<N, T> normal(const Vector<N, T>& point) const;

        const Vector<N, T>& org() const;
        const Vector<N, T>& e(unsigned n) const;
};

template <size_t N, typename T>
template <typename... P>
Rectangle<N, T>::Rectangle(const Vector<N, T>& org, const P&... vectors)
{
        static_assert((std::is_same_v<Vector<N, T>, P> && ...));
        static_assert(sizeof...(P) == N - 1);

        set_data(org, {{vectors...}});
}

template <size_t N, typename T>
Rectangle<N, T>::Rectangle(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors)
{
        set_data(org, vectors);
}

template <size_t N, typename T>
void Rectangle<N, T>::set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors)
{
        m_org = org;
        m_vectors = vectors;
        m_normal = normalize(ortho_nn(vectors));
        m_geometry.set_data(m_normal, m_org, m_vectors);
}

template <size_t N, typename T>
bool Rectangle<N, T>::intersect(const Ray<N, T>& r, T* t) const
{
        return m_geometry.intersect(r, m_org, m_normal, t);
}

template <size_t N, typename T>
Vector<N, T> Rectangle<N, T>::normal(const Vector<N, T>&) const
{
        return m_normal;
}

template <size_t N, typename T>
const Vector<N, T>& Rectangle<N, T>::org() const
{
        return m_org;
}

template <size_t N, typename T>
const Vector<N, T>& Rectangle<N, T>::e(unsigned n) const
{
        ASSERT(n < N - 1);
        return m_vectors[n];
}

template <typename Rectangle>
class RectangleAlgorithm final
{
        static constexpr size_t N = Rectangle::DIMENSION;
        using T = typename Rectangle::DataType;

        static_assert(N <= 30);

        static constexpr int VERTEX_COUNT = 1 << (N - 1);

        template <int n, typename F>
        static void vertices_impl(const Rectangle& p, const Vector<N, T>& org, const F& f)
        {
                if constexpr (n >= 0)
                {
                        vertices_impl<n - 1>(p, org, f);
                        vertices_impl<n - 1>(p, org + p.e(n), f);
                }
                else
                {
                        f(org);
                }
        }

        template <typename F>
        static void vertices(const Rectangle& p, const F& f)
        {
                constexpr int last_index = N - 2;

                // Смещаться по каждому измерению для перехода к другой вершине.

                vertices_impl<last_index>(p, p.org(), f);
        }

public:
        using Vertices = std::array<Vector<N, T>, VERTEX_COUNT>;

        static Vertices vertices(const Rectangle& r)
        {
                Vertices result;

                unsigned vertex_count = 0;

                vertices(r, [&vertex_count, &result](const Vector<N, T>& org) {
                        ASSERT(vertex_count < result.size());
                        result[vertex_count++] = org;
                });

                ASSERT(vertex_count == result.size());

                return result;
        }
};

template <typename Rectangle>
typename RectangleAlgorithm<Rectangle>::Vertices rectangle_vertices(const Rectangle& r)
{
        return RectangleAlgorithm<Rectangle>::vertices(r);
}
