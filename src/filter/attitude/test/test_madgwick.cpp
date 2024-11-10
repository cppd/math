/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <src/com/log.h>
#include <src/filter/attitude/madgwick.h>
#include <src/numerical/quaternion.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace ns::filter::attitude::test
{
namespace
{
template <typename T>
bool equal(const T a, const T b, const T precision)
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

template <typename T, typename P>
void test_equal(const T& a, const T& b, const P precision)
{
        for (std::size_t i = 0; i < std::tuple_size_v<T>; ++i)
        {
                if (!equal(a[i], b[i], precision))
                {
                        error(to_string(a) + " is not equal to " + to_string(b));
                }
        }
}

template <typename T>
void test_m(const T precision)
{
        numerical::Quaternion<T> q;

        const auto cmp = [&](const T w, const T x, const T y, const T z)
        {
                test_equal(q, {w, x, y, z}, precision);
        };

        const T beta = madgwick_beta<T>(1);
        Madgwick<T> m;

        for (int i = 0; i < 10; ++i)
        {
                q = m.update({0.01, 0.02, 0.03}, {3, 4, 5}, beta, 0.1, 1);
        }
        cmp(0.896883542158740787844L, 0.353210147162852465801L, -0.265415656397648120859L, 0.0199256890719475578416L);

        q = m.update({0.01, 0.02, 0.03}, {0, 0, 0}, beta, 0.1, 1);
        cmp(0.896940894562757041824L, 0.353239921590941529961L, -0.265038161415321047061L, 0.0217568942860156602685L);
}

template <typename T>
void test_mm(const T precision)
{
        numerical::Quaternion<T> q;

        const auto cmp = [&](const T w, const T x, const T y, const T z)
        {
                test_equal(q, {w, x, y, z}, precision);
        };

        const T beta = madgwick_beta<T>(1);
        const T zeta = madgwick_zeta<T>(0.1);
        MadgwickMarg<T> m;

        for (int i = 0; i < 10; ++i)
        {
                q = m.update({0.01, 0.02, 0.03}, {3, 4, 5}, {2, -3, 4}, beta, zeta, 0.1, 1, 1);
        }
        cmp(0.620363456233866526746L, 0.199558167398623417972L, 0.663642023000996199224L, 0.367294140031146965319L);

        q = m.update({0.01, 0.02, 0.03}, {0, 0, 0}, {2, -3, 4}, beta, zeta, 0.1, 1, 1);
        cmp(0.612400673687452607309L, 0.201650341705407511953L, 0.668516208234102618762L, 0.370659727898168788243L);

        q = m.update({0.01, 0.02, 0.03}, {3, 4, 5}, {0, 0, 0}, beta, zeta, 0.1, 1, 1);
        cmp(0.611696622987349623412L, 0.220555175307468252527L, 0.640826584894761140925L, 0.40807345437198153405L);

        q = m.update({0.01, 0.02, 0.03}, {0, 0, 0}, {0, 0, 0}, beta, zeta, 0.1, 1, 1);
        cmp(0.603648959038190618021L, 0.222166385960238515492L, 0.6456164487307030759L, 0.411618066087763750726L);
}

template <typename T>
void test_impl(const T precision)
{
        test_m<T>(precision);
        test_mm<T>(precision);
}

void test()
{
        LOG("Test Madgwick filter");
        test_impl<float>(1e-5);
        test_impl<double>(1e-14);
        test_impl<long double>(0);
        LOG("Test Madgwick filter passed");
}

TEST_SMALL("Madgwick", test)
}
}
