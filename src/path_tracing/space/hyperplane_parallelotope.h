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
#include "path_tracing/space/hyperplane_geometry.h"

template <size_t N, typename T>
class HyperplaneParallelotope
{
        static constexpr int VERTEX_COUNT = 1 << (N - 1);

        Vector<N, T> m_org;
        std::array<Vector<N, T>, N - 1> m_vectors;
        Vector<N, T> m_normal;
        HyperplaneParallelotopeGeometry<N, T> m_geometry;

        void set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors);

public:
        static constexpr size_t DIMENSION = N;
        using DataType = T;

        HyperplaneParallelotope() = default;

        template <typename... P>
        HyperplaneParallelotope(const Vector<N, T>& org, const P&... vectors);

        HyperplaneParallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors);

        bool intersect(const Ray<N, T>& r, T* t) const;

        Vector<N, T> normal(const Vector<N, T>& point) const;

        const Vector<N, T>& org() const;
        const Vector<N, T>& e(unsigned n) const;
};

template <size_t N, typename T>
template <typename... P>
HyperplaneParallelotope<N, T>::HyperplaneParallelotope(const Vector<N, T>& org, const P&... vectors)
{
        static_assert((std::is_same_v<Vector<N, T>, P> && ...));
        static_assert(sizeof...(P) == N - 1);

        set_data(org, {{vectors...}});
}

template <size_t N, typename T>
HyperplaneParallelotope<N, T>::HyperplaneParallelotope(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors)
{
        set_data(org, vectors);
}

template <size_t N, typename T>
void HyperplaneParallelotope<N, T>::set_data(const Vector<N, T>& org, const std::array<Vector<N, T>, N - 1>& vectors)
{
        m_org = org;
        m_vectors = vectors;
        m_normal = normalize(ortho_nn(vectors));
        m_geometry.set_data(m_normal, m_org, m_vectors);
}

template <size_t N, typename T>
bool HyperplaneParallelotope<N, T>::intersect(const Ray<N, T>& r, T* t) const
{
        return m_geometry.intersect(r, m_org, m_normal, t);
}

template <size_t N, typename T>
Vector<N, T> HyperplaneParallelotope<N, T>::normal(const Vector<N, T>&) const
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
