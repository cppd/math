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
#include <src/filter/attitude/kalman/quaternion.h>
#include <src/filter/attitude/kalman/ukf_utility.h>
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
                const Quaternion<T> q = Quaternion<T>(1.3, {-0.2, 0.3, -0.4}).normalized();
                const numerical::Vector<3, T> v = quaternion_to_error(q);
                const numerical::Vector<3, T> c(
                        -0.305404449748731134394L, 0.458106674623096659185L, -0.610808899497462268787L);
                test_equal(v, c, precision);
        }
        {
                const numerical::Vector<3, T> v(0.2, -0.3, 0.4);
                const Quaternion<T> q = error_to_quaternion(v);
                const Quaternion<T> c(
                        0.965396122567337429453L,
                        {0.0968541929606670444315L, -0.145281289441000553207L, 0.193708385921334088863L});
                test_equal(q, c, precision);
        }
}

void test()
{
        LOG("Test attitude UKF utility");
        test_impl<float>(1e-6);
        test_impl<double>(1e-15);
        test_impl<long double>(0);
        LOG("Test attitude UKF utility passed");
}

TEST_SMALL("Attitude UKF Utility", test)
}
}
