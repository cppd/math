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

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/filter/attitude/kalman/filter_imu.h>
#include <src/filter/attitude/kalman/filter_marg.h>
#include <src/filter/attitude/kalman/ukf_imu.h>
#include <src/filter/attitude/kalman/ukf_marg.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <optional>

namespace ns::filter::attitude::kalman::test
{
namespace
{
constexpr unsigned INIT_COUNT{10};

template <typename T>
void check_attitude(const numerical::Quaternion<T>& attitude)
{
        if (!attitude.is_unit())
        {
                error("Attitude " + to_string(attitude) + " is not unit");
        }
}

template <typename T>
void check_attitude(const std::optional<numerical::Quaternion<T>>& attitude)
{
        if (!attitude)
        {
                error("No attitude");
        }
        check_attitude(*attitude);
}

void check_bias(const auto& bias)
{
        if (!bias)
        {
                error("No bias");
        }
}

template <typename T>
void test_imu(const T precision)
{
        constexpr T INIT_VARIANCE = square(0.1);

        constexpr T DT = 0.01L;

        constexpr T VARIANCE_GYRO = square(1e-4);

        constexpr T VARIANCE_ACC = square(0.01);
        constexpr T VARIANCE_ACC_DIRECTION = square(0.01);

        const numerical::Vector<3, T> axis = numerical::Vector<3, T>(3, 5, 8).normalized();

        FilterImu<T, UkfImu> filter(INIT_COUNT, INIT_VARIANCE);

        for (int i = 0; i < 100; ++i)
        {
                filter.update_acc(axis * T{9.8}, VARIANCE_ACC, VARIANCE_ACC_DIRECTION);
                filter.update_gyro(axis * T{0.2}, axis * T{0.3}, VARIANCE_GYRO, DT);
                filter.update_gyro(axis * T{0.3}, axis * T{0.2}, VARIANCE_GYRO, DT);
        }

        const auto a = filter.attitude();
        check_attitude(a);
        ASSERT(a);

        test_equal(
                *a,
                numerical::Quaternion<T>(
                        {0.153107912344738161621L, -0.269255351726610421466L, -0.46693786244933079143L},
                        0.828268407797415102934L),
                precision);

        test_equal(
                numerical::rotate_vector(a->conjugate(), {0, 0, 1}),
                {0.303047640302402107495L, 0.505079930334457345253L, 0.808119045488030572047L}, precision);
}

template <typename T>
void test_marg(const T precision)
{
        constexpr T INIT_VARIANCE_ERROR = square(0.1);
        constexpr T INIT_VARIANCE_BIAS = square(0.1);

        constexpr T DT = 0.01L;

        constexpr T VARIANCE_GYRO_R = square(1e-3);
        constexpr T VARIANCE_GYRO_W = square(1e-2);

        constexpr T VARIANCE_ACC = square(0.01);
        constexpr T VARIANCE_MAG = square(0.01);

        const numerical::Vector<3, T> axis = numerical::Vector<3, T>(3, 5, 8).normalized();
        const numerical::Vector<3, T> mag{15, -20, 25};

        FilterMarg<T, UkfMarg> filter(INIT_COUNT, INIT_VARIANCE_ERROR, INIT_VARIANCE_BIAS);

        for (int i = 0; i < 1000; ++i)
        {
                filter.update_acc_mag(axis * T{9.8}, mag, VARIANCE_ACC, VARIANCE_MAG);
                const T k = 1 + i / T{1000};
                filter.update_gyro(axis * T{0.010} * k, axis * T{0.015} * k, VARIANCE_GYRO_R, VARIANCE_GYRO_W, DT);
                filter.update_gyro(axis * T{0.015} * k, axis * T{0.010} * k, VARIANCE_GYRO_R, VARIANCE_GYRO_W, DT);
        }

        const auto a = filter.attitude();
        check_attitude(a);
        ASSERT(a);

        test_equal(
                *a,
                numerical::Quaternion<T>(
                        {0.192756008969864434791L, 0.242454332156917833161L, 0.942639884540007082128L},
                        0.124463110594081722268L),
                precision);

        test_equal(
                numerical::rotate_vector(a->conjugate(), {0, 0, 1}),
                {0.303045763364969783054L, 0.505076272265423196226L, 0.808122035648705001399L}, precision);

        const auto b = filter.bias();
        check_bias(b);
        ASSERT(b);

        const numerical::Vector<3, T> bias{(*b)[0] / axis[0], (*b)[1] / axis[1], (*b)[2] / axis[2]};

        test_equal(bias, {0.0246333890965691297965L, 0.0246333879906535483539L, 0.0246333882931189850517L}, precision);
}

template <typename T>
void test_impl(const T precision)
{
        test_imu(precision);
        test_marg(precision);
}

void test()
{
        LOG("Test attitude UKF");
        test_impl<float>(1e-5);
        test_impl<double>(1e-14);
        test_impl<long double>(1e-18);
        LOG("Test attitude UKF passed");
}

TEST_SMALL("Attitude UKF", test)
}
}
