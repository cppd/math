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

#include "cmp.h"

#include <src/com/log.h>
#include <src/filter/attitude/ekf/cross_matrix.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

namespace ns::filter::attitude::ekf::test
{
namespace
{
template <typename T>
void test_impl(const T precision)
{
        using Vector3 = numerical::Vector<3, T>;
        using Matrix3 = numerical::Matrix<3, 3, T>;

        {
                const Vector3 v(1, 2, 3);
                const Matrix3 m = cross_matrix<1>(v);
                const Matrix3 c = {
                        { 0, -3,  2},
                        { 3,  0, -1},
                        {-2,  1,  0}
                };
                test_equal(m, c, precision);
        }

        const Vector3 v1(1, 2, 3);
        const Vector3 v2(3, -2, 1);

        test_equal(cross(v1, v2), cross_matrix<1>(v1) * v2, precision);

        const Matrix3 m = cross_matrix<1>(v1);
        test_equal(m * m, cross_matrix<2>(v1), precision);
        test_equal(m * m * m, cross_matrix<3>(v1), precision);
        test_equal(m * m * m * m, cross_matrix<4>(v1), precision);
        test_equal(m * m * m * m * m, cross_matrix<5>(v1), precision);
        test_equal(m * m * m * m * m * m, cross_matrix<6>(v1), precision);
}

void test()
{
        LOG("Test attitude EKF cross matrix");
        test_impl<float>(1e-5);
        test_impl<double>(1e-14);
        test_impl<long double>(0);
        LOG("Test attitude EKF cross matrix passed");
}

TEST_SMALL("Attitude EKF Cross Matrix", test)
}
}
