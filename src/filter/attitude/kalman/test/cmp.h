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

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/filter/attitude/kalman/quaternion.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace ns::filter::attitude::kalman::test
{
namespace cmp_implementation
{
template <typename T>
bool equal(const T a, const T b, const T precision)
{
        static_assert(std::is_floating_point_v<T>);

        if (a == b)
        {
                return true;
        }

        const T abs = std::abs(a - b);
        if (abs < precision)
        {
                return true;
        }

        const T rel = abs / std::max(std::abs(a), std::abs(b));
        return (rel < precision);
}

template <std::size_t N, typename T>
bool equal(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b, const T precision)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!equal(a[i], b[i], precision))
                {
                        return false;
                }
        }
        return true;
}
}

template <typename T>
        requires (std::is_floating_point_v<T>)
void test_equal(const T a, const T b, const T precision)
{
        namespace impl = cmp_implementation;

        if (!impl::equal(a, b, precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <std::size_t N, typename T>
void test_equal(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b, const T precision)
{
        namespace impl = cmp_implementation;

        if (!impl::equal(a, b, precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <typename T>
void test_equal(const Quaternion<T>& a, const Quaternion<T>& b, const T precision)
{
        namespace impl = cmp_implementation;

        if (!impl::equal(a.w(), b.w(), precision) || !impl::equal(a.vec(), b.vec(), precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <typename T>
void test_equal(const numerical::Quaternion<T>& a, const numerical::Quaternion<T>& b, const T precision)
{
        namespace impl = cmp_implementation;

        if (!impl::equal(a.w(), b.w(), precision) || !impl::equal(a.vec(), b.vec(), precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <std::size_t R, std::size_t C, typename T>
void test_equal(const numerical::Matrix<R, C, T>& a, const numerical::Matrix<R, C, T>& b, const T precision)
{
        namespace impl = cmp_implementation;

        for (std::size_t r = 0; r < R; ++r)
        {
                if (!impl::equal(a.row(r), b.row(r), precision))
                {
                        error(to_string(a) + " is not equal to " + to_string(b));
                }
        }
}
}
