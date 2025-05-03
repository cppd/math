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

#include <src/com/log.h>
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
        {
                const Quaternion<T> q = Quaternion<T>(-2, {5.2, -3.3, 4.4}).normalized();
                const numerical::Vector<3, T> v = global_to_local(q, {2.1, -3.2, 4.3});
                const numerical::Vector<3, T> c(
                        5.0222059063468757101L, -2.42440854951868102712L, 1.4281775167237726246L);
                test_equal(v, c, precision);
        }
        {
                const Quaternion<T> a = Quaternion<T>(-2, {5.2, -3.3, 4.4}).normalized();
                const Quaternion<T> b = Quaternion<T>(3, {-2.2, -5.3, 1.4}).normalized();
                const Quaternion<T> c = Quaternion<T>(
                        0.351101224971911535743L,
                        {-0.025064887010625194954L, -0.340496849698185567712L, -0.871872454323440007758L});
                test_equal(a * b, c, precision);
        }
        {
                const Quaternion<T> a = Quaternion<T>(2, {-2.2, 1.3, -2.4}).normalized();
                const numerical::Vector<3, T> b(-2.2, -5.3, 1.4);
                const Quaternion<T> c = Quaternion<T>(
                        1.34040711039905629403L,
                        {1.61047065020219336382L, -4.69761900428209103068L, -2.90380246467226329392L});
                test_equal(a * b, c, precision);
        }
        {
                const numerical::Vector<3, T> a(2.4, -5.1, 3.5);
                const Quaternion<T> b = Quaternion<T>(-3, {2.2, 2.7, -3.4}).normalized();
                const Quaternion<T> c = Quaternion<T>(
                        -3.56623341937517289012L,
                        {2.63925759187696671888L, 0.0979446157356596890445L, 4.93221100668856692224L});
                test_equal(a * b, c, precision);
        }

        const auto rotation_quaternion = [](const numerical::Vector<3, T>& axis, const T angle)
        {
                return Quaternion<T>(numerical::Quaternion<T>::rotation_quaternion(axis, angle));
        };

        {
                const Quaternion<T> q = rotation_quaternion({1, 0, 0}, T{1} / 10);
                const numerical::Vector<3, T> v(0, 1, 0);
                const numerical::Vector<3, T> r(0, 0.995004165278025766135L, -0.0998334166468281523107L);
                test_equal((q * v * q.conjugate()).vec(), r, precision);
        }
        {
                const Quaternion<T> q = rotation_quaternion({0, 1, 0}, T{1} / 10);
                const numerical::Vector<3, T> v(1, 0, 0);
                const numerical::Vector<3, T> r(0.995004165278025766135L, 0, 0.0998334166468281523107L);
                test_equal((q * v * q.conjugate()).vec(), r, precision);
        }
        {
                const Quaternion<T> q = rotation_quaternion({0, 0, 1}, T{1} / 10);
                const numerical::Vector<3, T> v(1, 0, 0);
                const numerical::Vector<3, T> r(0.995004165278025766135L, -0.0998334166468281523107L, 0);
                test_equal((q * v * q.conjugate()).vec(), r, precision);
        }
}

void test()
{
        LOG("Test attitude Kalman quaternion");
        test_impl<float>(1e-6);
        test_impl<double>(1e-15);
        test_impl<long double>(0);
        LOG("Test attitude Kalman quaternion passed");
}

TEST_SMALL("Attitude Kalman Quaternion", test)
}
}
