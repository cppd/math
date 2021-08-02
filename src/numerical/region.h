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

#include "vec.h"

#include <src/com/error.h>

namespace ns
{
template <std::size_t N, typename T>
class Region final
{
        Vector<N, T> m_offset0;
        Vector<N, T> m_extent;
        Vector<N, T> m_offset1;

public:
        Region() = default;

        Region(const Vector<N, T>& offset, const Vector<N, T>& extent)
                : m_offset0(offset), m_extent(extent), m_offset1(m_offset0 + m_extent)
        {
        }

        // offset[0], offset[1], ..., offset[N - 1], extent[0], extent[1], ..., extent[N - 1]
        template <typename... Types>
        Region(const Types&... v)
        {
                static_assert(sizeof...(Types) == 2 * N);
                int i = -1;
                ((++i < static_cast<int>(N) ? (m_offset0[i] = v) : (m_extent[i - N] = v)), ...);
                m_offset1 = m_offset0 + m_extent;
        }

        const Vector<N, T>& from() const
        {
                return m_offset0;
        }

        const Vector<N, T>& to() const
        {
                return m_offset1;
        }

        const Vector<N, T>& extent() const
        {
                return m_extent;
        }

        template <std::size_t X = N>
        T x0() const requires((X == 2 || X == 3) && X == N)
        {
                return m_offset0[0];
        }

        template <std::size_t X = N>
        T y0() const requires((X == 2 || X == 3) && X == N)
        {
                return m_offset0[1];
        }

        template <std::size_t X = N>
        T z0() const requires(X == 3 && X == N)
        {
                return m_offset0[2];
        }

        template <std::size_t X = N>
        T x1() const requires((X == 2 || X == 3) && X == N)
        {
                return m_offset1[0];
        }

        template <std::size_t X = N>
        T y1() const requires((X == 2 || X == 3) && X == N)
        {
                return m_offset1[1];
        }

        template <std::size_t X = N>
        T z1() const requires(X == 3 && X == N)
        {
                return m_offset1[2];
        }

        template <std::size_t X = N>
        T width() const requires((X == 2 || X == 3) && X == N)
        {
                return m_extent[0];
        }

        template <std::size_t X = N>
        T height() const requires((X == 2 || X == 3) && X == N)
        {
                return m_extent[1];
        }

        template <std::size_t X = N>
        T depth() const requires(X == 3 && X == N)
        {
                return m_extent[2];
        }

        template <typename Type>
        bool is_inside(const Vector<N, Type>& p) const
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if (p[i] < m_offset0[i] || p[i] >= m_offset1[i])
                        {
                                return false;
                        }
                }
                return true;
        }

        template <typename... Types>
        bool is_inside(const Types&... p) const
        {
                static_assert(sizeof...(Types) == N);
                int i = -1;
                return (((++i, p >= m_offset0[i] && p < m_offset1[i])) && ...);
        }

        bool is_positive() const
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if (m_offset0[i] < 0)
                        {
                                return false;
                        }
                        if (m_extent[i] <= 0)
                        {
                                return false;
                        }
                        ASSERT(m_offset0[i] + m_extent[i] == m_offset1[i]);
                }
                return true;
        }
};
}
