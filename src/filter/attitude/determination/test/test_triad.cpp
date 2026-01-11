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
#include <src/filter/attitude/determination/triad.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

namespace ns::filter::attitude::determination::test
{
namespace
{
template <typename T>
void test_impl(const T precision)
{
        const numerical::QuaternionHJ<T, true> q = numerical::QuaternionHJ<T, true>({1, -2, 3}, 4).normalized();

        const numerical::Vector<3, T> r1(-2, 3, -4);
        const numerical::Vector<3, T> r2(2, 3, -4);
        const numerical::Vector<3, T> s1 = numerical::rotate_vector(q, r1);
        const numerical::Vector<3, T> s2 = numerical::rotate_vector(q, r2);

        const numerical::QuaternionHJ<T, true> a = triad_attitude<T>({s1, s2}, {r1, r2});

        test_equal(a, q, precision);
}

void test()
{
        LOG("Test attitude determination triad");
        test_impl<float>(1e-7);
        test_impl<double>(1e-15);
        test_impl<long double>(1e-19);
        LOG("Test attitude determination triad passed");
}

TEST_SMALL("Attitude Determination Triad", test)
}
}
