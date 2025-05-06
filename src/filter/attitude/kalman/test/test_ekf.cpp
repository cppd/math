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
#include <src/filter/attitude/kalman/ekf_imu.h>
#include <src/filter/attitude/kalman/ekf_marg.h>
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
void test_imu(const T precision)
{
        constexpr T DT = 0.01L;

        constexpr T VARIANCE_GYRO = square(1e-4);

        constexpr T VARIANCE_ACC = square(0.01);
        constexpr T VARIANCE_ACC_DIRECTION = square(0.01);

        EkfImu<T> f;

        const numerical::Vector<3, T> axis = numerical::Vector<3, T>(3, 5, 8).normalized();

        for (int i = 0; i < 100; ++i)
        {
                f.update_acc(axis * T{9.8}, VARIANCE_ACC, VARIANCE_ACC_DIRECTION);
                f.update_gyro(axis * T{0.2}, axis * T{0.3}, VARIANCE_GYRO, DT);
                f.update_gyro(axis * T{0.3}, axis * T{0.2}, VARIANCE_GYRO, DT);
        }

        const auto a = f.attitude();

        check_attitude(a);
        ASSERT(a);

        test_equal(
                *a,
                numerical::Quaternion<T>(
                        0.82822937784681067595L,
                        {0.153083694947164173687L, -0.269266344945741933117L, -0.467008688883161158801L}),
                precision);

        test_equal(
                numerical::rotate_vector(a->conjugate(), {0, 0, 1}),
                {0.303045763365663224317L, 0.505076272276105374566L, 0.808122035641768599534L}, precision);
}

template <typename T>
void test_marg_1(const T precision)
{
        constexpr T DT = 0.01L;

        constexpr T VARIANCE_GYRO_R = square(1e-3);
        constexpr T VARIANCE_GYRO_W = square(1e-2);

        constexpr T VARIANCE_ACC = square(0.01);
        constexpr T VARIANCE_ACC_DIRECTION = square(0.01);

        constexpr T VARIANCE_MAG = square(0.01);
        constexpr T VARIANCE_MAG_DIRECTION = square(0.01);

        EkfMarg<T> f;

        const numerical::Vector<3, T> axis = numerical::Vector<3, T>(3, 5, 8).normalized();

        for (int i = 0; i < 1000; ++i)
        {
                f.update_acc(axis * T{9.8}, VARIANCE_ACC, VARIANCE_ACC_DIRECTION);
                f.update_mag({15, -20, 25}, VARIANCE_MAG, VARIANCE_MAG_DIRECTION);
                const T k = 1 + i / T{1000};
                f.update_gyro(axis * T{0.010} * k, axis * T{0.015} * k, VARIANCE_GYRO_R, VARIANCE_GYRO_W, DT);
                f.update_gyro(axis * T{0.015} * k, axis * T{0.010} * k, VARIANCE_GYRO_R, VARIANCE_GYRO_W, DT);
        }

        const auto a = f.attitude();

        check_attitude(a);
        ASSERT(a);

        test_equal(
                *a,
                numerical::Quaternion<T>(
                        0.124439467170343173159L,
                        {0.192749927647793641811L, 0.242459166811415931201L, 0.942643006037410423055L}),
                precision);

        test_equal(
                numerical::rotate_vector(a->conjugate(), {0, 0, 1}),
                {0.303045763365663224588L, 0.505076272276105374675L, 0.808122035641768599317L}, precision);

        const numerical::Vector<3, T> bias{f.bias()[0] / axis[0], f.bias()[1] / axis[1], f.bias()[2] / axis[2]};

        test_equal(bias, {0.0246948027225495865531L, 0.0246948027225495872205L, 0.0246948027225495878202L}, precision);
}

template <typename T>
void test_marg_2(const T precision)
{
        constexpr T DT = 0.01L;

        constexpr T VARIANCE_GYRO_R = square(1e-3);
        constexpr T VARIANCE_GYRO_W = square(1e-2);

        constexpr T VARIANCE_ACC = square(0.01);
        constexpr T VARIANCE_MAG = square(0.01);

        EkfMarg<T> f;

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
                numerical::Quaternion<T>(
                        0.124463110842182846114L,
                        {0.192756009035205944175L, 0.242454332112122312624L, 0.942639884505408993628L}),
                precision);

        test_equal(
                numerical::rotate_vector(a->conjugate(), {0, 0, 1}),
                {0.30304576336566322494L, 0.505076272276105374729L, 0.808122035641768599263L}, precision);

        const numerical::Vector<3, T> bias{f.bias()[0] / axis[0], f.bias()[1] / axis[1], f.bias()[2] / axis[2]};

        test_equal(bias, {0.0246334465758026264918L, 0.0246334465758026172981L, 0.0246334465758026190701L}, precision);
}

template <typename T>
void test_impl(const T precision)
{
        test_imu(precision);
        test_marg_1(precision);
        test_marg_2(precision);
}

void test()
{
        LOG("Test attitude EKF");
        test_impl<float>(1e-4);
        test_impl<double>(1e-13);
        test_impl<long double>(1e-17);
        LOG("Test attitude EKF passed");
}

TEST_SMALL("Attitude EKF", test)
}
}
