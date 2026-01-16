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
#include <src/com/random/pcg.h>
#include <src/filter/attitude/determination/quest.h>
#include <src/numerical/complement.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <array>

namespace ns::filter::attitude::determination::test
{
namespace
{
template <typename T>
numerical::Vector<3, T> add_error(const numerical::Vector<3, T>& v, const T error, PCG& pcg)
{
        const std::array<numerical::Vector<3, T>, 2> c =
                numerical::orthogonal_complement_of_unit_vector(v.normalized());

        const numerical::Vector<2, T> r = sampling::uniform_on_sphere<2, T>(pcg);

        return v + error * (c[0] * r[0] + c[1] * r[1]);
}

template <typename T>
void test_const(const T precision)
{
        const numerical::QuaternionHJ<T, true> q = numerical::QuaternionHJ<T, true>({1, -2, 3}, 4).normalized();

        {
                const numerical::Vector<3, T> r1(-2, 3, -4);
                const numerical::Vector<3, T> r2(2, 3, -4);
                const numerical::Vector<3, T> s1 = numerical::rotate_vector(q, r1);
                const numerical::Vector<3, T> s2 = numerical::rotate_vector(q, r2);

                const numerical::QuaternionHJ<T, true> a = quest_attitude<T>({s1, s2}, {r1, r2}, {0.5, 0.5});

                test_equal(a, q, precision);
        }

        {
                const numerical::Vector<3, T> r1(-2, 3, -4);
                const numerical::Vector<3, T> r2(2, 3, -4);
                const numerical::Vector<3, T> r3(-2, -3, -4);
                const numerical::Vector<3, T> s1 = numerical::rotate_vector(q, r1);
                const numerical::Vector<3, T> s2 = numerical::rotate_vector(q, r2);
                const numerical::Vector<3, T> s3 = numerical::rotate_vector(q, r3);

                const numerical::QuaternionHJ<T, true> a =
                        quest_attitude<T>({s1, s2, s3}, {r1, r2, r3}, {0.5, 0.5, 0.5});

                test_equal(a, q, precision);
        }
}

template <typename T>
void test_random()
{
        const numerical::QuaternionHJ<T, true> q = numerical::QuaternionHJ<T, true>({1, -2, 3}, 4).normalized();

        PCG pcg;

        numerical::Vector<3, T> r1;
        numerical::Vector<3, T> r2;
        numerical::Vector<3, T> r3;

        while (true)
        {
                r1 = sampling::uniform_on_sphere<3, T>(pcg);
                r2 = sampling::uniform_on_sphere<3, T>(pcg);
                r3 = sampling::uniform_on_sphere<3, T>(pcg);
                const T max = 0.98;
                if (std::abs(dot(r1, r2)) < max && std::abs(dot(r1, r3)) < max && std::abs(dot(r2, r3)) < max)
                {
                        break;
                }
        }

        const numerical::Vector<3, T> s1 = numerical::rotate_vector(q, r1);
        const numerical::Vector<3, T> s2 = numerical::rotate_vector(q, r2);
        const numerical::Vector<3, T> s3 = numerical::rotate_vector(q, r3);

        const T e1 = 0.01 * r1.norm();
        const T e2 = 0.02 * r2.norm();
        const T e3 = 0.1 * r3.norm();

        const numerical::Vector<3, T> s1e = add_error(s1, e1, pcg);
        const numerical::Vector<3, T> s2e = add_error(s2, e2, pcg);
        const numerical::Vector<3, T> s3e = add_error(s3, e3, pcg);

        const T w1 = r1.norm() / e1;
        const T w2 = r2.norm() / e2;
        const T w3 = r3.norm() / e3;

        const numerical::QuaternionHJ<T, true> a = quest_attitude<T>({s1e, s2e, s3e}, {r1, r2, r3}, {w1, w2, w3});

        const numerical::Vector<4, T> av(a.x(), a.y(), a.z(), a.w());
        const numerical::Vector<4, T> qv(q.x(), q.y(), q.z(), q.w());
        const T diff = (av - qv).norm();

        if (diff < 0.1)
        {
                return;
        }

        error(to_string(a) + " is not similar to " + to_string(q) + ", diff = " + to_string(diff) + ", cosines = "
              + to_string(dot(r1, r2)) + ", " + to_string(dot(r1, r3)) + ", " + to_string(dot(r2, r3)));
}

template <typename T>
void test_impl(const T precision)
{
        test_const(precision);
        test_random<T>();
}

void test()
{
        LOG("Test attitude determination quest");
        test_impl<float>(1e-6);
        test_impl<double>(1e-15);
        test_impl<long double>(1e-19);
        LOG("Test attitude determination quest passed");
}

TEST_SMALL("Attitude Determination Quest", test)
}
}
