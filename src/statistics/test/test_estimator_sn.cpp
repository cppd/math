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
#include <src/statistics/estimator_sn.h>
#include <src/statistics/median.h>
#include <src/test/test.h>

#include <cmath>
#include <cstddef>
#include <random>
#include <type_traits>
#include <vector>

namespace ns::statistics
{
namespace
{
template <typename T>
void compare(const T a, const T b, const T precision)
{
        if (!(std::abs(a - b) <= precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <typename T>
void test_constant(const std::type_identity_t<T> precision)
{
        {
                const std::vector<T> data{1, 4, -1, 15};

                const T sn = estimator_sn(data);
                compare(sn, T{5}, precision);

                const T sd = estimator_sn_standard_deviation(sn);
                compare(sd, T{5.9630000000000000001L}, precision);
        }
        {
                const std::vector<T> data{1, 4, -1, 15, -2};

                const T sn = estimator_sn(data);
                compare(sn, T{4.5}, precision);

                const T sd = estimator_sn_standard_deviation(sn);
                compare(sd, T{5.36670000000000000035L}, precision);
        }
}

template <typename T>
T estimator_sn_n2(const std::vector<T>& data)
{
        const std::size_t size = data.size();
        ASSERT(size > 1);

        std::vector<T> v;
        v.reserve(size);

        std::vector<T> t;
        t.reserve(size - 1);

        for (std::size_t i = 0; i < size; ++i)
        {
                t.clear();

                for (std::size_t j = 0; j < i; ++j)
                {
                        t.push_back(std::abs(data[i] - data[j]));
                }
                for (std::size_t j = i + 1; j < size; ++j)
                {
                        t.push_back(std::abs(data[i] - data[j]));
                }

                const T m = median(&t);
                v.push_back(m);
        }

        return median(&v);
}

template <typename T>
std::vector<T> make_data(const std::size_t count)
{
        constexpr std::size_t ERROR_COUNT = 10;
        constexpr T MEAN = -1;
        constexpr T STD_DEV = 10;

        PCG engine;
        std::normal_distribution<T> nd(MEAN, STD_DEV);
        std::uniform_int_distribution<unsigned> uid_size(0, 1);

        const std::size_t size = count + uid_size(engine);

        std::vector<T> res;
        res.reserve(size + ERROR_COUNT);

        for (std::size_t i = 0; i < size; ++i)
        {
                res.push_back(nd(engine));
        }

        for (std::size_t i = 1; i <= ERROR_COUNT; ++i)
        {
                res.push_back(MEAN + T{10'000} * i * STD_DEV);
        }

        return res;
}

template <typename T>
void test_random()
{
        const std::vector<T> data = make_data<T>(500);

        const T sn = estimator_sn(data);
        const T sn_n2 = estimator_sn_n2(data);

        compare(sn, sn_n2, T{0});
}

template <typename T>
void test_random_big()
{
        const std::vector<T> data = make_data<T>(10'000);

        const T sn = estimator_sn(data);
        const T sd = estimator_sn_standard_deviation(sn);

        if (!(sn > T{8.0} && sn < T{8.8}))
        {
                error("Scale " + to_string(sn) + " is out of range");
        }

        if (!(sd > T{9.5} && sd < T{10.5}))
        {
                error("Standard deviation " + to_string(sd) + " is out of range");
        }
}

template <typename T>
void test_impl(const std::type_identity_t<T> precision)
{
        test_constant<T>(precision);
        test_random<T>();
        test_random_big<T>();
}

void test()
{
        LOG("Test estimator Sn");

        test_impl<float>(1e-6);
        test_impl<double>(1e-15);
        test_impl<long double>(0);

        LOG("Test estimator Sn passed");
}

TEST_SMALL("Estimator Sn", test)
}
}
