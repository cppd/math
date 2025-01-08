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
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/filter/attitude/kalman/ukf_marg.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

namespace ns::filter::attitude::kalman::test
{
namespace
{
void check_attitude(const auto& attitude)
{
        if (!attitude)
        {
                error("No attitude");
        }

        if (!attitude->is_unit())
        {
                error("Attitude " + to_string(*attitude) + " is not unit");
        }
}

template <typename T>
void test_impl_marg(const T precision)
{
        constexpr T INIT_VARIANCE_R = square(0.1);
        constexpr T INIT_VARIANCE_W = square(0.1);

        constexpr T DT = 0.01L;

        constexpr T VARIANCE_GYRO_R = square(1e-3);
        constexpr T VARIANCE_GYRO_W = square(1e-2);

        constexpr T VARIANCE_ACC = square(0.01);
        constexpr T VARIANCE_MAG = square(0.01);

        UkfMarg<T> f(INIT_VARIANCE_R, INIT_VARIANCE_W);

        const numerical::Vector<3, T> axis = numerical::Vector<3, T>(3, 5, 8).normalized();

        for (int i = 0; i < 1000; ++i)
        {
                f.update_acc_mag(axis * T{9.8}, {15, -20, 25}, VARIANCE_ACC, VARIANCE_MAG);
                const T k = 1 + i / T{1000};
                f.update_gyro(axis * T{0.010} * k, axis * T{0.015} * k, VARIANCE_GYRO_R, VARIANCE_GYRO_W, DT);
                f.update_gyro(axis * T{0.015} * k, axis * T{0.010} * k, VARIANCE_GYRO_R, VARIANCE_GYRO_W, DT);
        }

        const auto a = f.attitude();

        check_attitude(a);
        ASSERT(a);

        test_equal(
                *a,
                {0.124466448620959271974L, 0.192756867536898622716L, 0.242453649581473038152L,
                 0.942639443790505141647L},
                precision);

        test_equal(
                numerical::rotate_vector(a->conjugate(), {0, 0, 1}),
                {0.303045763366371434283L, 0.505076272272152458067L, 0.808122035643973593479L}, precision);

        const numerical::Vector<3, T> bias{f.bias()[0] / axis[0], f.bias()[1] / axis[1], f.bias()[2] / axis[2]};

        test_equal(bias, {0.0242792783333279290384L, 0.0242792761205706998989L, 0.0242792767262187216555L}, precision);
}

template <typename T>
void test_impl(const T precision)
{
        test_impl_marg(precision);
}

void test()
{
        LOG("Test attitude UKF");
        test_impl<float>(1e-5);
        test_impl<double>(1e-13);
        test_impl<long double>(0);
        LOG("Test attitude UKF passed");
}

TEST_SMALL("Attitude UKF", test)
}
}
