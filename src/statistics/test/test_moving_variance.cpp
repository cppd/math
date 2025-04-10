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

#include "compare.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/numerical/vector.h>
#include <src/statistics/moving_variance.h>
#include <src/test/test.h>

#include <array>
#include <cmath>
#include <cstddef>

namespace ns::statistics::test
{
namespace
{
template <typename T>
        requires (std::is_floating_point_v<T>)
[[nodiscard]] T sqrt(const T a)
{
        return std::sqrt(a);
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Vector<N, T> sqrt(const numerical::Vector<N, T>& a)
{
        numerical::Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = std::sqrt(a[i]);
        }
        return res;
}

template <typename T>
void test(const T& precision)
{
        const auto cmp = [&](const T& a, const T& b)
        {
                compare(a, b, precision);
        };

        static constexpr std::size_t WINDOW_SIZE = 3;

        MovingVariance<T> variance(WINDOW_SIZE);

        if (variance.has_variance())
        {
                error("Variance is not empty");
        }

        if (variance.has_variance_n())
        {
                error("Variance is not empty");
        }

        if (!(variance.size() == 0))
        {
                error("Variance is not empty");
        }

        variance.push(T{1});

        if (variance.has_variance())
        {
                error("Variance has variance");
        }

        if (!variance.has_variance_n())
        {
                error("Variance is empty");
        }

        if (!(variance.size() == 1))
        {
                error("Variance data size " + to_string(variance.size()) + " is not equal to 1");
        }

        cmp(T{1}, variance.mean());
        cmp(T{0}, variance.variance_n());

        struct Data final
        {
                T value;
                T mean;
                T variance;
                T variance_n;
        };

        constexpr std::array DATA = std::to_array<Data>({
                { T{2},  T{3} / T{2},   T{1} / T{2},   T{1} / T{4}},
                {T{-2},  T{1} / T{3},  T{13} / T{3},  T{26} / T{9}},
                {T{10}, T{10} / T{3}, T{112} / T{3}, T{224} / T{9}},
                { T{3}, T{11} / T{3}, T{109} / T{3}, T{218} / T{9}},
                {T{-8},  T{5} / T{3}, T{247} / T{3}, T{494} / T{9}},
                { T{1}, T{-4} / T{3}, T{103} / T{3}, T{206} / T{9}},
                { T{9},  T{2} / T{3}, T{217} / T{3}, T{434} / T{9}}
        });

        for (const Data& d : DATA)
        {
                variance.push(d.value);

                if (!(variance.has_variance() && variance.has_variance_n()))
                {
                        error("Variance is empty");
                }

                cmp(d.mean, variance.mean());
                cmp(d.variance, variance.variance());
                cmp(d.variance_n, variance.variance_n());
                cmp(sqrt(d.variance), variance.standard_deviation());
                cmp(sqrt(d.variance_n), variance.standard_deviation_n());
        }

        if (!(variance.size() == 3))
        {
                error("Variance data size " + to_string(variance.size()) + " is not equal to "
                      + to_string(WINDOW_SIZE));
        }
}

void test_variance()
{
        LOG("Test moving variance");

        test<float>(1e-5);
        test<double>(1e-13);
        test<long double>(1e-17);

        test(numerical::Vector<3, float>(1e-5));
        test(numerical::Vector<3, double>(1e-13));
        test(numerical::Vector<3, long double>(1e-17));

        LOG("Test moving variance passed");
}

TEST_SMALL("Moving Variance", test_variance)
}
}
