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
#include <src/filter/attitude/kalman/constant.h>
#include <src/filter/attitude/kalman/integrator.h>
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
        constexpr T WT = W_THRESHOLD<T>;
        constexpr numerical::Vector<3, T> W_0(-WT / T{3}, WT * T{2}, -WT / T{2});
        constexpr numerical::Vector<3, T> W_1(-WT / T{3.2}, WT * T{2.2}, -WT / T{2.2});

        {
                const Quaternion<T> q0 = Quaternion<T>(-2, {3, -4, 5}).normalized();
                const Quaternion<T> q = zeroth_order_quaternion_integrator(q0, T{1000} * W_0, T{0.01});
                const Quaternion<T> c(
                        0.272087277903365161506L,
                        {-0.408198391225154576625L, 0.54435940156013223895L, -0.680452370513489564142L});
                test_equal(q, c, precision);
        }
        {
                const Quaternion<T> q0 = Quaternion<T>(-2, {3, -4, 5}).normalized();
                const Quaternion<T> q = zeroth_order_quaternion_integrator(q0, W_0 / T{100}, T{0.01});
                const Quaternion<T> c(
                        0.27216552619343278754L,
                        {-0.408248289964892883573L, 0.54433105423532311245L, -0.680413817825339523862L});
                test_equal(q, c, precision);
        }
        {
                const Quaternion<T> q0 = Quaternion<T>(-2, {3, -4, 5}).normalized();
                const Quaternion<T> q = first_order_quaternion_integrator(q0, T{1000} * W_0, T{2000} * W_1, T{0.01});
                const Quaternion<T> c(
                        0.272044680206759212409L,
                        {-0.408165114591462740046L, 0.544376512937662761791L, -0.680475674336394548548L});
                test_equal(q, c, precision);
        }
        {
                const Quaternion<T> q0 = Quaternion<T>(-2, {3, -4, 5}).normalized();
                const Quaternion<T> q = first_order_quaternion_integrator(q0, W_0 / T{100}, W_1 / T{200}, T{0.01});
                const Quaternion<T> c(
                        0.272165526380372617882L,
                        {-0.408248290068823516851L, 0.544331054171798492858L, -0.680413817739024907383L});
                test_equal(q, c, precision);
        }
}

void test()
{
        LOG("Test attitude Kalman integrator");
        test_impl<float>(1e-7);
        test_impl<double>(1e-15);
        test_impl<long double>(0);
        LOG("Test attitude Kalman integrator passed");
}

TEST_SMALL("Attitude Kalman Integrator", test)
}
}
