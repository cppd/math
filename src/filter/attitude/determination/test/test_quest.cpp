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

#include <src/com/benchmark.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/filter/attitude/determination/quest.h>
#include <src/numerical/complement.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>
#include <src/sampling/sphere_uniform.h>
#include <src/test/test.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

namespace ns::filter::attitude::determination::test
{
namespace
{
template <typename T>
using Quaternion = numerical::QuaternionHJ<T, true>;

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
        const Quaternion<T>& q,
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
void test_const(const Quaternion<T>& q, const T precision)
{
        ASSERT(q.is_normalized());

        {
                const numerical::Vector<3, T> r1(-2, 3, -4);
                const numerical::Vector<3, T> r2(2, 3, -4);

                const numerical::Vector<3, T> s1 = numerical::rotate_vector(q, r1);
                const numerical::Vector<3, T> s2 = numerical::rotate_vector(q, r2);

                const Quaternion<T> a = quest_attitude<T>({s1, s2}, {r1, r2}, {0.5, 0.5});

                test_equal(a, q, precision);
        }

        {
                const numerical::Vector<3, T> r1(-2, 3, -4);
                const numerical::Vector<3, T> r2(2, 3, -4);
                const numerical::Vector<3, T> r3(-2, -3, -4);

                const numerical::Vector<3, T> s1 = numerical::rotate_vector(q, r1);
                const numerical::Vector<3, T> s2 = numerical::rotate_vector(q, r2);
                const numerical::Vector<3, T> s3 = numerical::rotate_vector(q, r3);

                const Quaternion<T> a = quest_attitude<T>({s1, s2, s3}, {r1, r2, r3}, {0.5, 0.5, 0.5});

                test_equal(a, q, precision);
        }
}

template <typename T>
void test_random(
        const T max_norm_diff,
        const T max_reference_cosine,
        const std::vector<T>& errors,
        const Quaternion<T>& q,
        PCG& pcg)
{
        ASSERT(errors.size() >= 2);
        ASSERT(q.is_normalized());

        const std::vector<numerical::Vector<3, T>> references =
                create_references(errors.size(), max_reference_cosine, pcg);

        const std::vector<numerical::Vector<3, T>> observations = create_observations(references, errors, q, pcg);

        const std::vector<T> weights = errors_to_weights(errors);

        const Quaternion<T> a = quest_attitude<T>(observations, references, weights);

        test_similar(a, q, max_norm_diff);
}

template <typename T>
void test_random(PCG& pcg, const T max_norm_diff, const T max_reference_cosine, std::vector<T> errors)
{
        const std::vector<Quaternion<T>> quaternions{
                { {1, -2, 3}, 4},
                {{-4, 3, -2}, 1},
                {  {1, 0, 0}, 0},
                {  {0, 1, 0}, 0},
                {  {0, 0, 1}, 0},
                {  {0, 0, 0}, 1},
        };

        for (const auto& q : quaternions)
        {
                std::ranges::shuffle(errors, pcg);
                test_random<T>(max_norm_diff, max_reference_cosine, errors, q.normalized(), pcg);
        }

        for (int i = 0; i < 100; ++i)
        {
                const numerical::Vector<4, T> v = sampling::uniform_on_sphere<4, T>(pcg);
                const Quaternion<T> q({v[0], v[1], v[2]}, v[3]);
                std::ranges::shuffle(errors, pcg);
                test_random<T>(max_norm_diff, max_reference_cosine, errors, q.normalized(), pcg);
        }
}

template <typename T>
void test_impl(const T precision)
{
        test_const(Quaternion<T>({1, -2, 3}, 4).normalized(), precision);

        test_const(Quaternion<T>({1, 0, 0}, 0), precision);
        test_const(Quaternion<T>({0, 1, 0}, 0), precision);
        test_const(Quaternion<T>({0, 0, 1}, 0), precision);
        test_const(Quaternion<T>({0, 0, 0}, 1), precision);

        PCG pcg;

        constexpr T MAX_DIFF = 0.11;
        constexpr T MAX_COSINE = 0.98;

        test_random<T>(pcg, MAX_DIFF, MAX_COSINE, {0.01, 0.03});
        test_random<T>(pcg, MAX_DIFF, MAX_COSINE, {0.01, 0.02, 0.1});
        test_random<T>(pcg, MAX_DIFF, MAX_COSINE, {0.01, 0.02, 0.05, 0.2});
        test_random<T>(pcg, MAX_DIFF, MAX_COSINE, {0.01, 0.02, 0.05, 10.0});
        test_random<T>(pcg, MAX_DIFF, MAX_COSINE, {0.01, 0.02, 0.05, 0.2, 10.0});
}

template <typename T>
void test_quest_performance(PCG& pcg, const T max_reference_cosine, std::vector<T> errors)
{
        static constexpr int DATA_COUNT = 5'000;
        static constexpr int ITERATION_COUNT = 100;

        const numerical::Vector<4, T> v = sampling::uniform_on_sphere<4, T>(pcg);
        const Quaternion<T> q({v[0], v[1], v[2]}, v[3]);
        std::ranges::shuffle(errors, pcg);

        const std::vector<T> weights = errors_to_weights(errors);

        std::vector<std::vector<numerical::Vector<3, T>>> references;
        std::vector<std::vector<numerical::Vector<3, T>>> observations;

        for (int i = 0; i < DATA_COUNT; ++i)
        {
                references.push_back(create_references(errors.size(), max_reference_cosine, pcg));
                observations.push_back(create_observations(references.back(), errors, q, pcg));
        }

        const Clock::time_point start_time = Clock::now();
        for (int i = 0; i < DATA_COUNT; ++i)
        {
                for (int j = 0; j < ITERATION_COUNT; ++j)
                {
                        do_not_optimize(quest_attitude<T>(observations[i], references[i], weights));
                }
        }
        const auto performance = std::llround(DATA_COUNT * (ITERATION_COUNT / duration_from(start_time)));

        LOG(std::string("QUEST<") + type_name<T>() + ">: size = " + to_string(errors.size())
            + ", performance = " + to_string_digit_groups(performance) + " o/s");
}

template <typename T>
void test_quest_performance(PCG& pcg)
{
        constexpr T MAX_COSINE = 0.98;

        test_quest_performance<T>(pcg, MAX_COSINE, {0.01, 0.03});
        test_quest_performance<T>(pcg, MAX_COSINE, {0.01, 0.02, 0.1});
        test_quest_performance<T>(pcg, MAX_COSINE, {0.01, 0.02, 0.05, 0.2});
        test_quest_performance<T>(pcg, MAX_COSINE, {0.01, 0.02, 0.05, 0.2, 10.0});
}

void test()
{
        LOG("Test attitude determination quest");
        test_impl<float>(1e-6);
        test_impl<double>(1e-15);
        test_impl<long double>(1e-19);
        LOG("Test attitude determination quest passed");
}

void test_performance()
{
        PCG pcg;
        test_quest_performance<float>(pcg);
        test_quest_performance<double>(pcg);
        test_quest_performance<long double>(pcg);
}

TEST_SMALL("Attitude Determination Quest", test)
TEST_PERFORMANCE("Attitude Determination Quest", test_performance)
}
}
