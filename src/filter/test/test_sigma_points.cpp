/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../sigma_points.h"

#include <src/com/log.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>

namespace ns::filter::test
{
namespace
{
template <typename T>
[[nodiscard]] bool equal(const T a, const T b, const T precision)
{
        static_assert(std::is_floating_point_v<T>);

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

template <std::size_t N, typename T>
[[nodiscard]] bool equal(const Vector<N, T>& a, const Vector<N, T>& b, const T precision)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!equal(a[i], b[i], precision))
                {
                        return false;
                }
        }
        return true;
}

template <typename T>
void test_impl(const T precision)
{
        const auto cmp = [&](const auto& a, const auto& b)
        {
                if (!(equal(a, b, precision)))
                {
                        error(to_string(a) + " is not equal to " + to_string(b));
                }
        };

        const SigmaPoints<2, T> points(0.1L, 2, 1);

        cmp(points.wm(),
            Vector<5, T>(
                    -65.6666666666666667268L, 16.6666666666666666817L, 16.6666666666666666817L, 16.6666666666666666817L,
                    16.6666666666666666817L));

        cmp(points.wc(),
            Vector<5, T>(
                    -62.6766666666666667287L, 16.6666666666666666817L, 16.6666666666666666817L, 16.6666666666666666817L,
                    16.6666666666666666817L));

        {
                const Vector<2, T> x(-1, 2);
                const Matrix<2, 2, T> p{
                        {   1, 0.1L},
                        {0.1L,    1}
                };
                const std::array<Vector<2, T>, 5> s = points.points(x, p);
                cmp(s[0], Vector<2, T>(-1, 2));
                cmp(s[1], Vector<2, T>(-0.826794919243112270719L, 2.01732050807568877283L));
                cmp(s[2], Vector<2, T>(-1, 2.17233687939614085981L));
                cmp(s[3], Vector<2, T>(-1.17320508075688772934L, 1.98267949192431122707L));
                cmp(s[4], Vector<2, T>(-1, 1.8276631206038591403L));
        }

        {
                const Vector<2, T> x(1.1L, -2.2L);
                const Matrix<2, 2, T> p{
                        {    1, -0.2L},
                        {-0.2L,     1}
                };
                const std::array<Vector<2, T>, 5> s = points.points(x, p);
                cmp(s[0], Vector<2, T>(1.1L, -2.2L));
                cmp(s[1], Vector<2, T>(1.27320508075688772925L, -2.23464101615137754591L));
                cmp(s[2], Vector<2, T>(1.1L, -2.03029437251522859435L));
                cmp(s[3], Vector<2, T>(0.92679491924311227074L, -2.16535898384862245418L));
                cmp(s[4], Vector<2, T>(1.1L, -2.36970562748477140574L));
        }
}

void test()
{
        LOG("Test sigma points");
        test_impl<float>(1e-5);
        test_impl<double>(1e-15);
        test_impl<long double>(0);
        LOG("Test sigma points passed");
}

TEST_SMALL("Sigma Points", test)
}
}
