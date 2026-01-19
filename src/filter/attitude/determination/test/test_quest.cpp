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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/random/pcg.h>
#include <src/filter/attitude/determination/quest.h>
#include <src/numerical/complement.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <array>
#include <cstddef>
#include <vector>

namespace ns::filter::attitude::determination::test
{
namespace
{
template <typename T>
bool check_angles(const std::vector<numerical::Vector<3, T>>& r, const T max_cosine)
{
        const T size = r.size();
        for (std::size_t i = 0; i < size; ++i)
        {
                for (std::size_t j = i + 1; j < size; ++j)
                {
                        if (!(std::abs(dot(r[i], r[j])) < max_cosine))
                        {
                                return false;
                        }
                }
        }
        return true;
}

template <typename T>
numerical::Vector<3, T> add_error(const numerical::Vector<3, T>& v, const T error, PCG& pcg)
{
        const std::array<numerical::Vector<3, T>, 2> c =
                numerical::orthogonal_complement_of_unit_vector(v.normalized());

        const numerical::Vector<2, T> r = sampling::uniform_on_sphere<2, T>(pcg);

        return v + error * (c[0] * r[0] + c[1] * r[1]);
}

template <typename T>
void test_const(const numerical::QuaternionHJ<T, true>& q, const T precision)
{
        ASSERT(q.is_normalized());

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
void test_random(const T max_diff, const T max_cosine, const std::vector<T>& errors)
{
        ASSERT(errors.size() >= 2);

        const T size = errors.size();

        const numerical::QuaternionHJ<T, true> q = numerical::QuaternionHJ<T, true>({1, -2, 3}, 4).normalized();

        PCG pcg;

        std::vector<numerical::Vector<3, T>> r(size);
        while (true)
        {
                for (std::size_t i = 0; i < size; ++i)
                {
                        r[i] = sampling::uniform_on_sphere<3, T>(pcg);
                }
                if (check_angles(r, max_cosine))
                {
                        break;
                }
        }

        std::vector<numerical::Vector<3, T>> s(size);
        for (std::size_t i = 0; i < size; ++i)
        {
                const numerical::Vector<3, T> rotated = numerical::rotate_vector(q, r[i]);
                s[i] = add_error(rotated, errors[i], pcg);
        }

        std::vector<T> w(size);
        for (std::size_t i = 0; i < size; ++i)
        {
                w[i] = 1 / errors[i];
        }

        const numerical::QuaternionHJ<T, true> a = quest_attitude<T>(s, r, w);

        const numerical::Vector<4, T> av(a.x(), a.y(), a.z(), a.w());
        const numerical::Vector<4, T> qv(q.x(), q.y(), q.z(), q.w());

        const T diff = (av - qv).norm();

        if (diff < max_diff)
        {
                return;
        }

        error(to_string(a) + " is not similar to " + to_string(q) + ", diff = " + to_string(diff));
}

template <typename T>
void test_impl(const T precision)
{
        test_const(numerical::QuaternionHJ<T, true>({1, -2, 3}, 4).normalized(), precision);

        test_const(numerical::QuaternionHJ<T, true>({1, 0, 0}, 0), precision);
        test_const(numerical::QuaternionHJ<T, true>({0, 1, 0}, 0), precision);
        test_const(numerical::QuaternionHJ<T, true>({0, 0, 1}, 0), precision);
        test_const(numerical::QuaternionHJ<T, true>({0, 0, 0}, 1), precision);

        test_random<T>(0.1, 0.98, {0.01, 0.03});
        test_random<T>(0.1, 0.98, {0.01, 0.02, 0.1});
        test_random<T>(0.1, 0.98, {0.01, 0.02, 0.05, 0.2});
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
