/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/exponent.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/filter/attitude/ekf/ekf_imu.h>
#include <src/filter/attitude/ekf/ekf_marg.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace ns::filter::attitude::ekf::test
{
namespace
{
template <typename T>
        requires (std::is_floating_point_v<T>)
bool equal(const T a, const T b, const T precision)
{
        if (a == b)
        {
                return true;
        }
        const T abs = std::abs(a - b);
        if (abs < precision)
        {
                return true;
        }
        const T rel = abs / std::max(std::abs(a), std::abs(b));
        return (rel < precision);
}

template <typename T, typename P>
        requires (!std::is_floating_point_v<T>)
bool equal(const T& a, const T& b, const P precision)
{
        for (std::size_t i = 0; i < std::tuple_size_v<T>; ++i)
        {
                if (!equal(a[i], b[i], precision))
                {
                        return false;
                }
        }
        return true;
}

template <typename T, typename P>
bool equal(const numerical::Quaternion<T>& a, const numerical::Quaternion<T>& b, const P precision)
{
        if (!equal(a.w(), b.w(), precision))
        {
                return false;
        }
        if (!equal(a.vec(), b.vec(), precision))
        {
                return false;
        }
        return true;
}

template <typename T, typename P>
void test_equal(const T& a, const T& b, const P precision)
{
        if (!equal(a, b, precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <typename T>
void test_impl_imu(const T precision)
{
        EkfImu<T> f;

        constexpr T VARIANCE = square(1e-4L);
        constexpr T DT = 0.01L;

        const numerical::Vector<3, T> axis = numerical::Vector<3, T>(3, 5, 8).normalized();

        for (int i = 0; i < 100; ++i)
        {
                f.update_acc(axis * T{9.8});
                f.update_gyro(axis * T{0.2}, axis * T{0.3}, VARIANCE, DT);
                f.update_gyro(axis * T{0.3}, axis * T{0.2}, VARIANCE, DT);
        }

        const auto a = f.attitude();

        if (!a)
        {
                error("No attitude");
        }

        if (!a->is_unit())
        {
                error("Attitude is not unit");
        }

        test_equal(
                *a,
                {0.82822937784681067595L, 0.153083694947164173687L, -0.269266344945741933117L,
                 -0.467008688883161158801L},
                precision);

        test_equal(
                numerical::rotate_vector(a->conjugate(), {0, 0, 1}),
                {0.303045763365663224317L, 0.505076272276105374566L, 0.808122035641768599534L}, precision);
}

template <typename T>
void test_impl_marg(const T precision)
{
        EkfMarg<T> f;

        constexpr T VARIANCE_R = square(1e-3L);
        constexpr T VARIANCE_W = square(5e-3L);
        constexpr T DT = 0.01L;

        const numerical::Vector<3, T> axis = numerical::Vector<3, T>(3, 5, 8).normalized();

        for (int i = 0; i < 1000; ++i)
        {
                f.update_acc(axis * T{9.8});
                f.update_mag({15, -20, 25});
                f.update_gyro(axis * T{0.010}, axis * T{0.015}, VARIANCE_R, VARIANCE_W, DT);
                f.update_gyro(axis * T{0.015}, axis * T{0.010}, VARIANCE_R, VARIANCE_W, DT);
        }

        const auto a = f.attitude();

        if (!a)
        {
                error("No attitude");
        }

        if (!a->is_unit())
        {
                error("Attitude is not unit");
        }

        test_equal(
                *a,
                {0.124510580868338503701L, 0.19276821849243910823L, 0.242444624849398093324L, 0.942633615501120782792L},
                precision);

        test_equal(
                numerical::rotate_vector(a->conjugate(), {0, 0, 1}),
                {0.303045763365665830576L, 0.505076272276098761692L, 0.808122035641771755158L}, precision);

        test_equal(
                f.bias(), {0.0037880682131818025904L, 0.00631344702195024136448L, 0.0101015152351109069228L},
                precision);
}

template <typename T>
void test_impl(const T precision)
{
        test_impl_imu(precision);
        test_impl_marg(precision);
}

void test()
{
        LOG("Test attitude EKF");
        test_impl<float>(1e-5);
        test_impl<double>(1e-14);
        test_impl<long double>(1e-20);
        LOG("Test attitude EKF passed");
}

TEST_SMALL("Attitude EKF", test)
}
}
