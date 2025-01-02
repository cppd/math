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

You should have received q copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "cmp.h"

#include <src/com/log.h>
#include <src/filter/attitude/kalman/ekf_utility.h>
#include <src/filter/attitude/kalman/quaternion.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

namespace ns::filter::attitude::kalman::test
{
namespace
{
template <typename T>
void test_impl(const T precision)
{
        {
                const numerical::Vector<3, T> v(-0.2L, 0.3L, -0.4L);
                const Quaternion<T> q = ekf_delta_quaternion(v);
                const Quaternion<T> c(0.842614977317635863055L, {-0.2L, 0.3L, -0.4L});
                test_equal(q, c, precision);
        }
        {
                const numerical::Vector<3, T> v(-1.2, 2.3, -3.4);
                const Quaternion<T> q = ekf_delta_quaternion(v);
                const Quaternion<T> c(
                        0.227684720124393468466L,
                        {-0.273221664149272152055L, 0.523674856286104937033L, -0.774128048422937772562L});
                test_equal(q, c, precision);
        }
}

void test()
{
        LOG("Test attitude EKF utility");
        test_impl<float>(1e-6);
        test_impl<double>(1e-15);
        test_impl<long double>(0);
        LOG("Test attitude EKF utility passed");
}

TEST_SMALL("Attitude EKF Utility", test)
}
}
