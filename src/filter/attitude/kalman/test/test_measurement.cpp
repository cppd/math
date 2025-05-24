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

#include "cmp.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/filter/attitude/kalman/init_utility.h>
#include <src/filter/attitude/kalman/measurement.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

namespace ns::filter::attitude::kalman::test
{
namespace
{
template <typename T>
void test_impl(const T precision)
{
        const numerical::Vector<3, T> z = numerical::Vector<3, T>(1, -2, 3);
        const numerical::Matrix<3, 3, T> attitude = initial_quaternion(z).rotation_matrix();

        {
                const numerical::Vector<3, T> m = numerical::Vector<3, T>(2, 1, -4).normalized();
                const T variance = 0.1;
                const auto mag = mag_measurement(attitude, m, variance);
                if (!mag)
                {
                        error("No mag measurement");
                }
                const numerical::Vector<3, T> c(
                        0.872871560943969525108L, -0.218217890235992381223L, -0.436435780471984762472L);
                test_equal(mag->y, c, precision);
                test_equal(mag->variance, T{0.196000000000000010938L}, precision);
        }
        {
                const numerical::Vector<3, T> m = numerical::Vector<3, T>(1.1, -2.1, 3.1).normalized();
                const T variance = 0.1;
                const auto mag = mag_measurement(attitude, m, variance);
                if (mag)
                {
                        error("Mag measurement");
                }
        }
}

void test()
{
        LOG("Test attitude Kalman measurement");
        test_impl<float>(1e-6);
        test_impl<double>(1e-15);
        test_impl<long double>(0);
        LOG("Test attitude Kalman measurement passed");
}

TEST_SMALL("Attitude Kalman Measurement", test)
}
}
