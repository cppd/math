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
                const Quaternion<T> q = Quaternion<T>::rotation_quaternion({1, 0, 0}, T{1} / 10);
                const numerical::Vector<3, T> v(0, 1, 0);
                const numerical::Vector<3, T> r(0, 0.995004165278025766135L, -0.0998334166468281523107L);
                test_equal((q * v * q.conjugate()).vec(), r, precision);
        }
        {
                const Quaternion<T> q = Quaternion<T>::rotation_quaternion({0, 1, 0}, T{1} / 10);
                const numerical::Vector<3, T> v(1, 0, 0);
                const numerical::Vector<3, T> r(0.995004165278025766135L, 0, 0.0998334166468281523107L);
                test_equal((q * v * q.conjugate()).vec(), r, precision);
        }
        {
                const Quaternion<T> q = Quaternion<T>::rotation_quaternion({0, 0, 1}, T{1} / 10);
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
