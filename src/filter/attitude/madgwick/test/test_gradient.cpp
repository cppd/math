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
#include <src/filter/attitude/madgwick/gradient.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector_object.h>
#include <src/test/test.h>

namespace ns::filter::attitude::madgwick::test
{
namespace
{
template <typename T>
void test_impl(const T precision)
{
        {
                const numerical::Quaternion<T> q = numerical::Quaternion<T>(5, {2, -2, 3}).normalized();
                const numerical::Vector<3, T> an = numerical::Vector<3, T>(-3, 4, -5).normalized();
                const numerical::Quaternion<T> g = compute_gn(q, an);
                const numerical::Quaternion<T> c(
                        0.303434274196882529867L,
                        {-0.677645761238154244925L, -0.327742977001662833285L, 0.584216059814056349199L});
                test_equal(g, c, precision);
        }
        {
                const numerical::Quaternion<T> q = numerical::Quaternion<T>(5, {2, -2, 3}).normalized();
                const numerical::Vector<3, T> an = numerical::Vector<3, T>(-3, 4, -5).normalized();
                const numerical::Vector<3, T> mn = numerical::Vector<3, T>(4, -2, 7).normalized();
                const numerical::Vector<2, T> b = numerical::Vector<2, T>(2, -3).normalized();
                const numerical::Quaternion<T> g = compute_gn(q, an, mn, b[0], b[1]);
                const numerical::Quaternion<T> c(
                        0.487434372410802660072L,
                        {-0.512295551244976688887L, -0.485803177052928420175L, 0.513766750512694707559L});
                test_equal(g, c, precision);
        }
        {
                const numerical::Quaternion<T> q = numerical::Quaternion<T>(1, {0, 0, 0}).normalized();
                const numerical::Vector<3, T> an = numerical::Vector<3, T>(0, 0, 1).normalized();
                const numerical::Quaternion<T> g = compute_gn(q, an);
                const numerical::Quaternion<T> c(0, {0, 0, 0});
                test_equal(g, c, precision);
        }
        {
                const numerical::Quaternion<T> q = numerical::Quaternion<T>(1, {0, 0, 0}).normalized();
                const numerical::Vector<3, T> an = numerical::Vector<3, T>(0, 0, 1).normalized();
                const numerical::Vector<3, T> mn = numerical::Vector<3, T>(1, 0, 0).normalized();
                const numerical::Vector<2, T> b = numerical::Vector<2, T>(1, 0).normalized();
                const numerical::Quaternion<T> g = compute_gn(q, an, mn, b[0], b[1]);
                const numerical::Quaternion<T> c(0, {0, 0, 0});
                test_equal(g, c, precision);
        }
}

void test()
{
        LOG("Test attitude Madgwick gradient");
        test_impl<float>(1e-6);
        test_impl<double>(1e-15);
        test_impl<long double>(0);
        LOG("Test attitude Madgwick gradient passed");
}

TEST_SMALL("Attitude Madgwick Gradient", test)
}
}
