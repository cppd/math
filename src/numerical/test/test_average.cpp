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
#include <src/numerical/average.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <array>
#include <cmath>
#include <cstddef>

namespace ns::numerical
{
namespace
{
template <typename T>
        requires (std::is_floating_point_v<T>)
void compare(const T a, const T b, const T precision)
{
        if (!(std::abs(a - b) <= precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <std::size_t N, typename T>
void compare(const Vector<N, T>& a, const Vector<N, T>& b, const Vector<N, T>& precision)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(std::abs(a[i] - b[i]) <= precision[i]))
                {
                        error(to_string(a) + " is not equal to " + to_string(b));
                }
        }
}

template <typename T>
void test(const T& precision)
{
        const auto cmp = [&](const T& a, const T& b)
        {
                compare(a, b, precision);
        };

        static constexpr std::size_t WINDOW_SIZE = 3;

        MovingAverage<T> average(WINDOW_SIZE);

        if (average.has_average())
        {
                error("Average is not empty");
        }

        if (!(average.size() == 0))
        {
                error("Average is not empty");
        }

        average.push(T{1});

        if (!average.has_average())
        {
                error("Average is empty");
        }

        if (!(average.size() == 1))
        {
                error("Average data size " + to_string(average.size()) + " is not equal to 1");
        }

        cmp(T{1}, average.average());

        struct Data final
        {
                T value;
                T mean;
        };

        constexpr std::array DATA = std::to_array<Data>({
                { T{2},  T{3} / T{2}},
                {T{-2},  T{1} / T{3}},
                {T{10}, T{10} / T{3}},
                { T{3}, T{11} / T{3}},
                {T{-8},  T{5} / T{3}},
                { T{1}, T{-4} / T{3}},
                { T{9},  T{2} / T{3}}
        });

        for (const Data& d : DATA)
        {
                average.push(d.value);

                if (!(average.has_average()))
                {
                        error("Average is empty");
                }

                cmp(d.mean, average.average());
        }

        if (!(average.size() == 3))
        {
                error("Average data size " + to_string(average.size()) + " is not equal to " + to_string(WINDOW_SIZE));
        }
}

void test_variance()
{
        LOG("Test average");

        test<float>(1e-6);
        test<double>(1e-15);
        test<long double>(1e-18);

        test(Vector<3, float>(1e-6));
        test(Vector<3, double>(1e-15));
        test(Vector<3, long double>(1e-18));

        LOG("Test average passed");
}

TEST_SMALL("Average", test_variance)
}
}
