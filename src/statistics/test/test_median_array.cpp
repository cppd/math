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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/statistics/median_array.h>
#include <src/test/test.h>

#include <algorithm>
#include <cstddef>
#include <random>
#include <tuple>
#include <vector>

namespace ns::statistics
{
namespace
{
template <typename T>
void compare(const T& a, const T& b)
{
        if (!(a == b))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <typename T>
void test_constant()
{
        compare(median_of_sorted_data<T>({}, {2}), T{2});
        compare(median_of_sorted_data<T>({2}, {}), T{2});
        compare(median_of_sorted_data<T>({}, {2, 3}), T{2.5});
        compare(median_of_sorted_data<T>({}, {1, 2, 3}), T{2});
        compare(median_of_sorted_data<T>({1}, {2}), T{1.5});
        compare(median_of_sorted_data<T>({2}, {1}), T{1.5});
        compare(median_of_sorted_data<T>({1}, {1, 1}), T{1});
        compare(median_of_sorted_data<T>({1}, {1, 2}), T{1});
        compare(median_of_sorted_data<T>({1}, {1, 3}), T{1});
        compare(median_of_sorted_data<T>({1, 2}, {1, 2}), T{1.5});
        compare(median_of_sorted_data<T>({1}, {1, 2, 3}), T{1.5});
        compare(median_of_sorted_data<T>({3, 4}, {1, 2}), T{2.5});
        compare(median_of_sorted_data<T>({1, 2}, {3, 4}), T{2.5});
        compare(median_of_sorted_data<T>({1}, {2, 3, 4}), T{2.5});
        compare(median_of_sorted_data<T>({1, 2}, {3, 4, 5}), T{3});
        compare(median_of_sorted_data<T>({1, 2, 3}, {4, 5}), T{3});
        compare(median_of_sorted_data<T>({1, 2}, {0, 3}), T{1.5});
        compare(median_of_sorted_data<T>({1, 2}, {0, 3, 4}), T{2});
        compare(median_of_sorted_data<T>({1, 4}, {0, 2, 3}), T{2});
}

template <typename T>
std::vector<T> make_sorted_data(PCG& pcg)
{
        const int min = std::uniform_int_distribution<int>(-10, 10)(pcg);
        const int max = std::uniform_int_distribution<int>(min, 10)(pcg);

        std::vector<T> res(std::uniform_int_distribution<unsigned>(1, 20)(pcg));

        std::uniform_int_distribution<int> uid(min, max);
        for (T& v : res)
        {
                v = uid(pcg);
        }

        std::ranges::sort(res);
        return res;
}

template <typename T>
T median_of_sorted_vector(const std::vector<T>& data)
{
        ASSERT(!data.empty());
        const std::size_t s = data.size();
        if (s & 1u)
        {
                return data[s / 2];
        }
        return (data[s / 2 - 1] + data[s / 2]) / 2;
}

template <typename T>
std::tuple<std::vector<T>, std::vector<T>> sample_two_vectors(const std::vector<T>& data, const std::size_t p, PCG& pcg)
{
        ASSERT(p <= data.size());

        std::tuple<std::vector<T>, std::vector<T>> res;
        std::size_t rest = data.size();
        std::size_t n = p;
        std::size_t i = 0;

        for (; n > 0; ++i)
        {
                ASSERT(i < data.size());
                const std::size_t r = std::uniform_int_distribution<std::size_t>(0, --rest)(pcg);
                if (r < n)
                {
                        std::get<0>(res).push_back(data[i]);
                        --n;
                }
                else
                {
                        std::get<1>(res).push_back(data[i]);
                }
        }

        for (; i < data.size(); ++i)
        {
                std::get<1>(res).push_back(data[i]);
        }

        ASSERT(std::get<0>(res).size() == p);
        ASSERT(std::get<1>(res).size() == data.size() - p);

        return res;
}

template <typename T>
void test_random()
{
        PCG pcg;

        const std::vector<T> data = make_sorted_data<T>(pcg);
        const T m = median_of_sorted_vector(data);
        const std::size_t p = std::uniform_int_distribution<std::size_t>(0, data.size())(pcg);

        {
                const std::vector<T> v1(data.begin(), data.begin() + p);
                const std::vector<T> v2(data.begin() + p, data.end());
                compare(median_of_sorted_data(v1, v2), m);
        }
        {
                const auto [v1, v2] = sample_two_vectors(data, p, pcg);
                compare(median_of_sorted_data(v1, v2), m);
        }
}

template <typename T>
void test()
{
        test_constant<T>();

        for (int i = 0; i < 10; ++i)
        {
                test_random<T>();
        }
}

void test_median()
{
        LOG("Test median array");

        test<float>();
        test<double>();
        test<long double>();

        LOG("Test median array passed");
}

TEST_SMALL("Median Array", test_median)
}
}
