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

#include <algorithm>
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
std::vector<numerical::Vector<3, T>> create_references(const std::size_t count, const T max_cosine, PCG& pcg)
{
        std::vector<numerical::Vector<3, T>> res(count);
        while (true)
        {
                for (std::size_t i = 0; i < count; ++i)
                {
                        res[i] = sampling::uniform_on_sphere<3, T>(pcg);
                }
                if (check_angles(res, max_cosine))
                {
                        break;
                }
        }
        return res;
}

template <typename T>
std::vector<numerical::Vector<3, T>> create_observations(
        const std::vector<numerical::Vector<3, T>>& references,
        const std::vector<T>& errors,
        const numerical::QuaternionHJ<T, true>& q,
        PCG& pcg)
{
        ASSERT(references.size() == errors.size());
        const T size = references.size();

        std::vector<numerical::Vector<3, T>> res;
        res.reserve(size);
        for (std::size_t i = 0; i < size; ++i)
        {
                const numerical::Vector<3, T> rotated = numerical::rotate_vector(q, references[i]);
                res.push_back(add_error(rotated, errors[i], pcg));
        }
        return res;
}

template <typename T>
std::vector<T> errors_to_weights(const std::vector<T>& errors)
{
        std::vector<T> weights;
        weights.reserve(errors.size());
        for (const T error : errors)
        {
                weights.push_back(1 / error);
        }
        return weights;
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
void test_random(
        const T max_diff,
        const T max_cosine,
        const std::vector<T>& errors,
        const numerical::QuaternionHJ<T, true>& q,
        PCG& pcg)
{
        ASSERT(errors.size() >= 2);
        ASSERT(q.is_normalized());

        const std::vector<numerical::Vector<3, T>> references = create_references(errors.size(), max_cosine, pcg);
        const std::vector<numerical::Vector<3, T>> observations = create_observations(references, errors, q, pcg);
        const std::vector<T> weights = errors_to_weights(errors);

        const numerical::QuaternionHJ<T, true> a = quest_attitude<T>(observations, references, weights);

        const numerical::Vector<4, T> av(a.x(), a.y(), a.z(), a.w());
        const numerical::Vector<4, T> qv(q.x(), q.y(), q.z(), q.w());

        const T diff_1 = (av - qv).norm();
        const T diff_2 = (av + qv).norm();

        if (diff_1 < max_diff || diff_2 < max_diff)
        {
                return;
        }

        error(to_string(a) + " is not similar to " + to_string(q) + ", diff 1 = " + to_string(diff_1)
              + ", diff 2 = " + to_string(diff_2));
}

template <typename T>
void test_random(const T max_diff, const T max_cosine, std::vector<T> errors)
{
        const std::vector<numerical::QuaternionHJ<T, true>> quaternions{
                { {1, -2, 3}, 4},
                {{-4, 3, -2}, 1},
                {  {1, 0, 0}, 0},
                {  {0, 1, 0}, 0},
                {  {0, 0, 1}, 0},
                {  {0, 0, 0}, 1},
        };

        PCG pcg;

        for (const auto& q : quaternions)
        {
                std::ranges::shuffle(errors, pcg);
                test_random<T>(max_diff, max_cosine, errors, q.normalized(), pcg);
        }

        for (int i = 0; i < 100; ++i)
        {
                const numerical::Vector<4, T> v = sampling::uniform_on_sphere<4, T>(pcg);
                const numerical::QuaternionHJ<T, true> q({v[0], v[1], v[2]}, v[3]);
                std::ranges::shuffle(errors, pcg);
                test_random<T>(max_diff, max_cosine, errors, q.normalized(), pcg);
        }
}

template <typename T>
void test_impl(const T precision)
{
        test_const(numerical::QuaternionHJ<T, true>({1, -2, 3}, 4).normalized(), precision);

        test_const(numerical::QuaternionHJ<T, true>({1, 0, 0}, 0), precision);
        test_const(numerical::QuaternionHJ<T, true>({0, 1, 0}, 0), precision);
        test_const(numerical::QuaternionHJ<T, true>({0, 0, 1}, 0), precision);
        test_const(numerical::QuaternionHJ<T, true>({0, 0, 0}, 1), precision);

        test_random<T>(0.11, 0.98, {0.01, 0.03});
        test_random<T>(0.11, 0.98, {0.01, 0.02, 0.1});
        test_random<T>(0.11, 0.98, {0.01, 0.02, 0.05, 0.2});
        test_random<T>(0.11, 0.98, {0.01, 0.02, 0.05, 10.0});
        test_random<T>(0.11, 0.98, {0.01, 0.02, 0.05, 0.2, 10.0});
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
