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
        Vector<N, T> offset0_;
        Vector<N, T> extent_;
        Vector<N, T> offset1_;

public:
        Region() = default;

        Region(const Vector<N, T>& offset, const Vector<N, T>& extent)
                : offset0_(offset), extent_(extent), offset1_(offset0_ + extent_)
        {
        }

        // offset[0], offset[1], ..., offset[N - 1], extent[0], extent[1], ..., extent[N - 1]
        template <typename... Types>
        Region(const Types&... v)
        {
                static_assert(sizeof...(Types) == 2 * N);
                int i = -1;
                ((++i < static_cast<int>(N) ? (offset0_[i] = v) : (extent_[i - N] = v)), ...);
                offset1_ = offset0_ + extent_;
        }

        const Vector<N, T>& from() const
        {
                return offset0_;
        }

        const Vector<N, T>& to() const
        {
                return offset1_;
        }

        const Vector<N, T>& extent() const
        {
                return extent_;
        }

        template <std::size_t X = N>
        T x0() const requires((X == 2 || X == 3) && X == N)
        {
                return offset0_[0];
        }

        template <std::size_t X = N>
        T y0() const requires((X == 2 || X == 3) && X == N)
        {
                return offset0_[1];
        }

        template <std::size_t X = N>
        T z0() const requires(X == 3 && X == N)
        {
                return offset0_[2];
        }

        template <std::size_t X = N>
        T x1() const requires((X == 2 || X == 3) && X == N)
        {
                return offset1_[0];
        }

        template <std::size_t X = N>
        T y1() const requires((X == 2 || X == 3) && X == N)
        {
                return offset1_[1];
        }

        template <std::size_t X = N>
        T z1() const requires(X == 3 && X == N)
        {
                return offset1_[2];
        }

        template <std::size_t X = N>
        T width() const requires((X == 2 || X == 3) && X == N)
        {
                return extent_[0];
        }

        template <std::size_t X = N>
        T height() const requires((X == 2 || X == 3) && X == N)
        {
                return extent_[1];
        }

        template <std::size_t X = N>
        T depth() const requires(X == 3 && X == N)
        {
                return extent_[2];
        }

        template <typename Type>
        bool is_inside(const Vector<N, Type>& p) const
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if (p[i] < offset0_[i] || p[i] >= offset1_[i])
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
                return (((++i, p >= offset0_[i] && p < offset1_[i])) && ...);
        }

        bool is_positive() const
        {
                for (unsigned i = 0; i < N; ++i)
                {
                        if (offset0_[i] < 0)
                        {
                                return false;
                        }
                        if (extent_[i] <= 0)
                        {
                                return false;
                        }
                        ASSERT(offset0_[i] + extent_[i] == offset1_[i]);
                }
                return true;
        }
};
}
