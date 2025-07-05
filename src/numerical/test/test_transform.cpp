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
#include <src/numerical/matrix.h>
#include <src/numerical/transform.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <type_traits>

namespace ns::numerical
{
namespace
{
template <typename T>
        requires (std::is_floating_point_v<T>)
bool equal(const T a, const T b, const T precision)
{
        if (a == b)
        {
                return true;
        }
        const T abs = std::abs(a - b);
        if (abs < precision)
        {
                return true;
        }
        const T rel = abs / std::max(std::abs(a), std::abs(b));
        return (rel < precision);
}

template <typename T, typename P>
        requires (!std::is_floating_point_v<T>)
bool equal(const T& a, const T& b, const P precision)
{
        for (std::size_t i = 0; i < std::tuple_size_v<T>; ++i)
        {
                if (!equal(a[i], b[i], precision))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t R, std::size_t C, typename T>
bool equal(const Matrix<R, C, T>& a, const Matrix<R, C, T>& b, const T precision)
{
        for (std::size_t r = 0; r < R; ++r)
        {
                if (!equal(a.row(r), b.row(r), precision))
                {
                        return false;
                }
        }
        return true;
}

template <typename T, typename P>
void test_equal(const T& a, const T& b, const P precision)
{
        if (!equal(a, b, precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <typename T>
void test(const T precision)
{
        {
                const Matrix<4, 4, T> m{
                        {2,  0, 0, 0},
                        {0, -3, 0, 0},
                        {0,  0, 4, 0},
                        {0,  0, 0, 1}
                };

                test_equal(transform::scale<T>(2, -3, 4), m, precision);
        }
        {
                const Matrix<4, 4, T> m{
                        {1, 0, 0, -3},
                        {0, 1, 0,  4},
                        {0, 0, 1, -2},
                        {0, 0, 0,  1}
                };

                test_equal(transform::translate<T>(-3, 4, -2), m, precision);
        }
        {
                const Matrix<4, 4, T> m{
                        { 1,  -2,  3,  -4},
                        {-5,   6, -7,   8},
                        { 9, -10, 11, -12},
                        { 0,   0,  0,   1}
                };

                const transform::MatrixVectorMultiplier multiplier(m);

                test_equal(multiplier({-3, 4, -2}), {-21, 61, -101}, precision);
        }
}

void test_quaternion()
{
        LOG("Test transform");
        test<float>(0);
        test<double>(0);
        test<long double>(0);
        LOG("Test transform passed");
}

TEST_SMALL("Transform", test_quaternion)
}
}
