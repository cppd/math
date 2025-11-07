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
#include <src/filter/attitude/kalman/init_imu.h>
#include <src/filter/attitude/kalman/init_marg.h>
#include <src/filter/attitude/kalman/quaternion.h>
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
template <typename T>
void check_attitude(const numerical::Quaternion<T>& attitude)
{
        if (!attitude.is_unit())
        {
                error("Attitude " + to_string(attitude) + " is not unit");
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

        std::optional<Quaternion<T>> init_q;
        InitImu<T> init_imu;
        do
        {
                init_q = init_imu.update(axis * T{9.8});
        } while (!init_q);

        UkfImu<T> f(*init_q, INIT_VARIANCE);

        for (int i = 0; i <= 90; ++i)
        {
                f.update_acc(axis * T{9.8}, VARIANCE_ACC, VARIANCE_ACC_DIRECTION);
                f.update_gyro(axis * T{0.2}, axis * T{0.3}, VARIANCE_GYRO, DT);
                f.update_gyro(axis * T{0.3}, axis * T{0.2}, VARIANCE_GYRO, DT);
        }

        const numerical::Quaternion<T> a = f.attitude();

        check_attitude(a);

        test_equal(
                a,
                numerical::Quaternion<T>(
                        0.828268407797415102825L,
                        {0.153107912344738161594L, -0.269255351726610421629L, -0.466937862449330791511L}),
                precision);

        test_equal(
                numerical::rotate_vector(a.conjugate(), {0, 0, 1}),
                {0.303047640302402107712L, 0.505079930334457345362L, 0.80811904548803057183L}, precision);
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

        std::optional<Quaternion<T>> init_q;
        InitMarg<T> init_marg;
        do
        {
                init_q = init_marg.update_acc_mag(axis * T{9.8}, mag);
        } while (!init_q);

        UkfMarg<T> f(*init_q, INIT_VARIANCE_ERROR, INIT_VARIANCE_BIAS);

        for (int i = 9; i < 1000; ++i)
        {
                f.update_acc_mag(axis * T{9.8}, mag, VARIANCE_ACC, VARIANCE_MAG);
                const T k = 1 + i / T{1000};
                f.update_gyro(axis * T{0.010} * k, axis * T{0.015} * k, VARIANCE_GYRO_R, VARIANCE_GYRO_W, DT);
                f.update_gyro(axis * T{0.015} * k, axis * T{0.010} * k, VARIANCE_GYRO_R, VARIANCE_GYRO_W, DT);
        }

        const numerical::Quaternion<T> a = f.attitude();

        check_attitude(a);

        test_equal(
                a,
                numerical::Quaternion<T>(
                        0.124463110594081722302L,
                        {0.192756008969864434804L, 0.242454332156917833161L, 0.942639884540007082128L}),
                precision);

        test_equal(
                numerical::rotate_vector(a.conjugate(), {0, 0, 1}),
                {0.303045763364969783054L, 0.50507627226542319628L, 0.808122035648705001399L}, precision);

        const numerical::Vector<3, T> bias{f.bias()[0] / axis[0], f.bias()[1] / axis[1], f.bias()[2] / axis[2]};

        test_equal(bias, {0.0246333890965691296593L, 0.0246333879906535484132L, 0.024633388293118985199L}, precision);
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
