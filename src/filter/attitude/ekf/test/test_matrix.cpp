/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/log.h>
#include <src/filter/attitude/ekf/matrix.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace ns::filter::attitude::ekf::test
{
namespace
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
void test_equal(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b, const T precision)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!equal(a[i], b[i], precision))
                {
                        error(to_string(a) + " is not equal to " + to_string(b));
                }
        }
}

template <std::size_t R, std::size_t C, typename T>
void test_equal(const numerical::Matrix<R, C, T>& a, const numerical::Matrix<R, C, T>& b, const T precision)
{
        for (std::size_t i = 0; i < R; ++i)
        {
                test_equal(a.row(i), b.row(i), precision);
        }
}

template <typename T>
void test_impl(const T precision)
{
        const numerical::Vector<3, T> v1{1, 2, 3};
        const numerical::Vector<3, T> v2{3, -2, 1};

        test_equal(cross(v1, v2), cross_matrix<1>(v1) * v2, precision);

        const numerical::Matrix<3, 3, T> m = cross_matrix<1>(v1);
        test_equal(m * m, cross_matrix<2>(v1), precision);
        test_equal(m * m * m, cross_matrix<3>(v1), precision);
        test_equal(m * m * m * m, cross_matrix<4>(v1), precision);
        test_equal(m * m * m * m * m, cross_matrix<5>(v1), precision);
        test_equal(m * m * m * m * m * m, cross_matrix<6>(v1), precision);
}

void test()
{
        LOG("Test attitude EKF matrix");
        test_impl<float>(0);
        test_impl<double>(0);
        test_impl<long double>(0);
        LOG("Test attitude EKF matrix passed");
}

TEST_SMALL("Attitude EKF Matrix", test)
}
}
