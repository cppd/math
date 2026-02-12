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

#pragma once

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/numerical/quaternion.h>

namespace ns::filter::attitude::determination::test
{
template <typename T, bool JPL>
void test_equal(const numerical::QuaternionHJ<T, JPL>& a, const numerical::QuaternionHJ<T, JPL>& b, const T precision)
{
        const T d_1 = (a - b).norm();
        const T d_2 = (a + b).norm();

        if (d_1 <= precision || d_2 <= precision)
        {
                return;
        }

        error(to_string(a) + " is not equal to " + to_string(b) + ", diff 1 = " + to_string(d_1)
              + ", diff 2 = " + to_string(d_2));
}
}
