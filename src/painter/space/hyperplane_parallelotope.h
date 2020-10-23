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

#include "hyperplane_geometry.h"

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

        Vector<N, T> m_org;
        std::array<Vector<N, T>, N - 1> m_vectors;
        Vector<N, T> m_normal;
        HyperplaneParallelotopeGeometry<N, T> m_geometry;

        void set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors);

public:
        using Vertices = std::array<Vector<N, T>, VERTEX_COUNT>;

        template <typename... P>
        explicit HyperplaneParallelotope(const Vector<N, T>& org, const P&... vectors);

        HyperplaneParallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors);

        bool intersect(const Ray<N, T>& r, T* t) const;

        const Vector<N, T>& normal(const Vector<N, T>& point) const;
        const Vector<N, T>& org() const;
        const Vector<N, T>& e(unsigned n) const;
};

template <size_t N, typename T>
template <typename... P>
HyperplaneParallelotope<N, T>::HyperplaneParallelotope(const Vector<N, T>& org, const P&... vectors)
{
        static_assert((std::is_same_v<Vector<N, T>, P> && ...));
        static_assert(sizeof...(P) == N - 1);

        set_data(org, {vectors...});
}

template <size_t N, typename T>
HyperplaneParallelotope<N, T>::HyperplaneParallelotope(
        const Vector<N, T>& org,
        const std::array<Vector<N, T>, N - 1>& vectors)
{
        set_data(org, vectors);
}

template <size_t N, typename T>
void HyperplaneParallelotope<N, T>::set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors)
{
        m_org = org;
        m_vectors = vectors;
        m_normal = ortho_nn(vectors).normalized();
        m_geometry.set_data(m_normal, m_org, m_vectors);
}

template <size_t N, typename T>
bool HyperplaneParallelotope<N, T>::intersect(const Ray<N, T>& r, T* t) const
{
        return m_geometry.intersect(r, m_org, m_normal, t);
}

template <size_t N, typename T>
const Vector<N, T>& HyperplaneParallelotope<N, T>::normal(const Vector<N, T>&) const
{
        return m_normal;
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

namespace hyperplane_parallelotope_implementation
{
template <int INDEX, size_t N, typename T, typename F>
void vertices(const HyperplaneParallelotope<N, T>& p, const Vector<N, T>& org, const F& f)
{
        if constexpr (INDEX >= 0)
        {
                vertices<INDEX - 1>(p, org, f);
                vertices<INDEX - 1>(p, org + p.e(INDEX), f);
        }
        else
        {
                f(org);
        }
}
}

template <size_t N, typename T>
typename HyperplaneParallelotope<N, T>::Vertices hyperplane_parallelotope_vertices(
        const HyperplaneParallelotope<N, T>& p)
{
        namespace impl = hyperplane_parallelotope_implementation;

        typename HyperplaneParallelotope<N, T>::Vertices result;

        unsigned vertex_count = 0;

        auto f = [&vertex_count, &result](const Vector<N, T>& org) {
                ASSERT(vertex_count < result.size());
                result[vertex_count++] = org;
        };

        // Смещаться по каждому измерению для перехода к другой вершине.
        constexpr int LAST_INDEX = N - 2;
        impl::vertices<LAST_INDEX>(p, p.org(), f);

        ASSERT(vertex_count == result.size());

        return result;
}
}
