/*
Copyright (C) 2017-2026 Topological Manifold

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
                const numerical::Vector<3, T> v(0.2, -0.3, 0.4);
                const Quaternion<T> center = Quaternion<T>(0.3, {-0.2, 0.1, -0.5}).normalized();
                const Quaternion<T> q = error_to_quaternion(v, center);
                const Quaternion<T> c(
                        0.673133918337381338399L,
                        {-0.347946905362277922438L, 0.0692871417083943385982L, -0.648862829129900554946L});
                test_equal(q, c, precision);
        }
        {
                const Quaternion<T> q = Quaternion<T>(1.5, {-0.4, 0.1, 0.6}).normalized();
                const Quaternion<T> center_inversed = Quaternion<T>(-2.1, {0.1, -0.5, 0.3}).normalized();
                const numerical::Vector<3, T> v = quaternion_to_error(q, center_inversed);
                const numerical::Vector<3, T> c(
                        -0.402911489752197424512L, 0.695938027753795459499L, 0.610471954169996024718L);
                test_equal(v, c, precision);
        }
}

void test()
{
        LOG("Test attitude UKF utility");
        test_impl<float>(1e-6);
        test_impl<double>(1e-15);
        test_impl<long double>(1e-19);
        LOG("Test attitude UKF utility passed");
}

TEST_SMALL("Attitude UKF Utility", test)
}
}
