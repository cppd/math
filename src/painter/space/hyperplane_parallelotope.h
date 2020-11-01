/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "hyperplane.h"

#include <src/com/error.h>
#include <src/numerical/orthogonal.h>
#include <src/numerical/ray.h>
#include <src/numerical/vec.h>

#include <array>

namespace painter
{
template <size_t N, typename T>
class HyperplaneParallelotope final
{
        static_assert(N <= 30);
        static constexpr int VERTEX_COUNT = 1 << (N - 1);

        struct Planes
        {
                Vector<N, T> n;
                T d;
        };
        std::array<Planes, N - 1> m_planes;

        Vector<N, T> m_org;
        std::array<Vector<N, T>, N - 1> m_vectors;
        Vector<N, T> m_normal;

        template <int INDEX, typename F>
        void vertices_impl(const Vector<N, T>& org, const F& f) const;

public:
        template <typename... P>
        explicit HyperplaneParallelotope(const Vector<N, T>& org, const P&... vectors);

        HyperplaneParallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors);

        bool intersect(const Ray<N, T>& r, T* t) const;

        const Vector<N, T>& normal(const Vector<N, T>& point) const;

        std::array<Vector<N, T>, VERTEX_COUNT> vertices() const;

        const Vector<N, T>& org() const;
        const Vector<N, T>& e(unsigned n) const;
};

template <size_t N, typename T>
template <typename... P>
HyperplaneParallelotope<N, T>::HyperplaneParallelotope(const Vector<N, T>& org, const P&... vectors)
        : HyperplaneParallelotope(org, {vectors...})
{
}

template <size_t N, typename T>
HyperplaneParallelotope<N, T>::HyperplaneParallelotope(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N - 1>& vectors)
{
        m_org = org;
        m_vectors = vectors;
        m_normal = ortho_nn(vectors).normalized();

        for (unsigned i = 0; i < N - 1; ++i)
        {
                std::swap(m_normal, m_vectors[i]);
                m_planes[i].n = ortho_nn(m_vectors);
                std::swap(m_normal, m_vectors[i]);

                if (dot(m_planes[i].n, m_vectors[i]) < 0)
                {
                        m_planes[i].n = -m_planes[i].n;
                }

                m_planes[i].d = dot(m_org, m_planes[i].n);

                // Относительное расстояние от вершины до плоскости должно быть равно 1
                T distance = dot(m_org + m_vectors[i], m_planes[i].n) - m_planes[i].d;
                ASSERT(distance >= 0);
                m_planes[i].n /= distance;
                m_planes[i].d /= distance;
        }
}

template <size_t N, typename T>
bool HyperplaneParallelotope<N, T>::intersect(const Ray<N, T>& r, T* t) const
{
        if (!hyperplane_intersect(r, m_org, m_normal, t))
        {
                return false;
        }

        Vector<N, T> intersection_point = r.point(*t);

        for (unsigned i = 0; i < N - 1; ++i)
        {
                // Относительное расстояние от грани до точки является координатой точки
                T d = dot(intersection_point, m_planes[i].n) - m_planes[i].d;
                if (d <= 0 || d >= 1)
                {
                        return false;
                }
        }

        return true;
}

template <size_t N, typename T>
const Vector<N, T>& HyperplaneParallelotope<N, T>::normal(const Vector<N, T>&) const
{
        return m_normal;
}

template <size_t N, typename T>
template <int INDEX, typename F>
void HyperplaneParallelotope<N, T>::vertices_impl(const Vector<N, T>& p, const F& f) const
{
        if constexpr (INDEX >= 0)
        {
                vertices_impl<INDEX - 1>(p, f);
                vertices_impl<INDEX - 1>(p + m_vectors[INDEX], f);
        }
        else
        {
                f(p);
        }
}

template <size_t N, typename T>
std::array<Vector<N, T>, HyperplaneParallelotope<N, T>::VERTEX_COUNT> HyperplaneParallelotope<N, T>::vertices() const
{
        std::array<Vector<N, T>, VERTEX_COUNT> result;

        unsigned count = 0;
        auto f = [&count, &result](const Vector<N, T>& p) {
                ASSERT(count < result.size());
                result[count++] = p;
        };

        vertices_impl<N - 2>(m_org, f);

        ASSERT(count == result.size());

        return result;
}

template <size_t N, typename T>
const Vector<N, T>& HyperplaneParallelotope<N, T>::org() const
{
        return m_org;
}

template <size_t N, typename T>
const Vector<N, T>& HyperplaneParallelotope<N, T>::e(unsigned n) const
{
        ASSERT(n < N - 1);
        return m_vectors[n];
}
}
