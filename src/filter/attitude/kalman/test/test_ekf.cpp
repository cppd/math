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
#include <src/filter/attitude/kalman/filter_imu.h>
#include <src/filter/attitude/kalman/filter_marg.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

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

template <typename T>
void test_imu(const T precision)
{
        constexpr T INIT_VARIANCE = square(0.1);

        constexpr T DT = 0.01L;

        constexpr T VARIANCE_GYRO = square(1e-4);

        constexpr T VARIANCE_ACC = square(0.01);
        constexpr T VARIANCE_ACC_DIRECTION = square(0.01);

        const numerical::Vector<3, T> axis = numerical::Vector<3, T>(3, 5, 8).normalized();

        FilterImu<T, EkfImu> filter(INIT_COUNT, INIT_VARIANCE);

        for (int i = 0; i < 100; ++i)
        {
                filter.update_acc(axis * T{9.8}, VARIANCE_ACC, VARIANCE_ACC_DIRECTION);
                filter.update_gyro(axis * T{0.2}, axis * T{0.3}, VARIANCE_GYRO, DT);
                filter.update_gyro(axis * T{0.3}, axis * T{0.2}, VARIANCE_GYRO, DT);
        }

        const auto a = filter.attitude();

        check_attitude(a);

        test_equal(
                *a,
                numerical::Quaternion<T>(
                        {0.153083694947164173673L, -0.269266344945741933252L, -0.467008688883161158909L},
                        0.828229377846810675841L),
                precision);

        test_equal(
                numerical::rotate_vector(a->conjugate(), {0, 0, 1}),
                {0.303045763365663224425L, 0.505076272276105374729L, 0.808122035641768599425L}, precision);
}

template <typename T>
void test_marg_1(const T precision)
{
        constexpr T INIT_VARIANCE_ERROR = square(0.1);
        constexpr T INIT_VARIANCE_BIAS = square(0.1);

        constexpr T DT = 0.01L;

        constexpr T VARIANCE_GYRO_R = square(1e-3);
        constexpr T VARIANCE_GYRO_W = square(1e-2);

        constexpr T VARIANCE_ACC = square(0.01);
        constexpr T VARIANCE_ACC_DIRECTION = square(0.01);

        constexpr T VARIANCE_MAG = square(0.01);
        constexpr T VARIANCE_MAG_DIRECTION = square(0.01);

        const numerical::Vector<3, T> axis = numerical::Vector<3, T>(3, 5, 8).normalized();
        const numerical::Vector<3, T> mag{15, -20, 25};

        FilterMarg<T, EkfMarg> filter(INIT_COUNT, INIT_VARIANCE_ERROR, INIT_VARIANCE_BIAS);

        for (int i = 0; i < 1000; ++i)
        {
                filter.update_acc(axis * T{9.8}, VARIANCE_ACC, VARIANCE_ACC_DIRECTION);
                filter.update_mag(mag, VARIANCE_MAG, VARIANCE_MAG_DIRECTION);
                const T k = 1 + i / T{1000};
                filter.update_gyro(axis * T{0.010} * k, axis * T{0.015} * k, VARIANCE_GYRO_R, VARIANCE_GYRO_W, DT);
                filter.update_gyro(axis * T{0.015} * k, axis * T{0.010} * k, VARIANCE_GYRO_R, VARIANCE_GYRO_W, DT);
        }

        const numerical::Quaternion<T> a = filter.attitude();

        check_attitude(a);

        test_equal(
                a,
                numerical::Quaternion<T>(
                        {0.192749927644040618149L, 0.242459166814399506272L, 0.942643006039336620863L},
                        0.124439467155752007197L),
                precision);

        test_equal(
                numerical::rotate_vector(a.conjugate(), {0, 0, 1}),
                {0.303045763365663225455L, 0.505076272276105374729L, 0.8081220356417685991L}, precision);

        const numerical::Vector<3, T> bias{
                filter.bias()[0] / axis[0], filter.bias()[1] / axis[1], filter.bias()[2] / axis[2]};

        test_equal(bias, {0.0246948026765289834482L, 0.0246948026765289776121L, 0.0246948026765289808359L}, precision);
}

template <typename T>
void test_marg_2(const T precision)
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

        FilterMarg<T, EkfMarg> filter(INIT_COUNT, INIT_VARIANCE_ERROR, INIT_VARIANCE_BIAS);

        for (int i = 0; i < 1000; ++i)
        {
                filter.update_acc_mag(axis * T{9.8}, mag, VARIANCE_ACC, VARIANCE_MAG);
                const T k = 1 + i / T{1000};
                filter.update_gyro(axis * T{0.010} * k, axis * T{0.015} * k, VARIANCE_GYRO_R, VARIANCE_GYRO_W, DT);
                filter.update_gyro(axis * T{0.015} * k, axis * T{0.010} * k, VARIANCE_GYRO_R, VARIANCE_GYRO_W, DT);
        }

        const numerical::Quaternion<T> a = filter.attitude();

        check_attitude(a);

        test_equal(
                a,
                numerical::Quaternion<T>(
                        {0.192756009035205943687L, 0.242454332112122312949L, 0.942639884505408993899L},
                        0.124463110842182844271L),
                precision);

        test_equal(
                numerical::rotate_vector(a.conjugate(), {0, 0, 1}),
                {0.303045763365663224967L, 0.50507627227610537462L, 0.808122035641768599263L}, precision);

        const numerical::Vector<3, T> b = filter.bias();

        const numerical::Vector<3, T> bias{b[0] / axis[0], b[1] / axis[1], b[2] / axis[2]};

        test_equal(bias, {0.0246334465758026123311L, 0.0246334465758026084195L, 0.0246334465758026095325L}, precision);
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
        test_impl<long double>(0);
        LOG("Test attitude EKF passed");
}

TEST_SMALL("Attitude EKF", test)
}
}
