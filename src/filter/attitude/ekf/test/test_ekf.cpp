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
#include <src/filter/attitude/ekf/ekf.h>
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
void test_impl_a(const T precision)
{
        Ekf<T> f;

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
                {0.828229377846810675896L, 0.153083694947164173714L, -0.269266344945741933144L,
                 -0.467008688883161158855L},
                precision);

        test_equal(
                numerical::rotate_vector(a->conjugate(), {0, 0, 1}),
                {0.303045763365663224263L, 0.50507627227610537462L, 0.808122035641768599479L}, precision);
}

template <typename T>
void test_impl_b(const T /*precision*/)
{
        EkfB<T> f;

        f.update_acc({0, 0, 0});
        f.update_mag({0, 0, 0});
        f.update_gyro({0, 0, 0}, {0, 0, 0}, 0, 0, 0);

        static_cast<void>(f.attitude());
        static_cast<void>(f.bias());
}

template <typename T>
void test_impl(const T precision)
{
        test_impl_a(precision);
        test_impl_b(precision);
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
