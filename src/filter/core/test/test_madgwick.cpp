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
#include <src/filter/core/madgwick.h>
#include <src/numerical/quaternion.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace ns::filter::core::test
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
void test_impl(const T precision)
{
        const T beta = madgwick_beta<T>(1);
        const T zeta = madgwick_zeta<T>(0.1);

        {
                Madgwick<T> m;
                numerical::Quaternion<T> q;
                for (int i = 0; i <= 10; ++i)
                {
                        q = m.update({0.01, 0.02, 0.03}, {3, 4, 5}, beta, 0.1, 1);
                }
                test_equal(
                        q,
                        {0.925845007236751315932L, 0.306664637927562528535L, -0.22005178876596579639L,
                         0.0185750553279297316612L},
                        precision);
        }
        {
                MadgwickMarg<T> m;
                numerical::Quaternion<T> q;
                for (int i = 0; i < 10; ++i)
                {
                        q = m.update({0.01, 0.02, 0.03}, {3, 4, 5}, {2, -3, 4}, beta, zeta, 0.1, 1, 1);
                }
                test_equal(
                        q,
                        {0.620363456233866526746L, 0.199558167398623417972L, 0.663642023000996199224L,
                         0.367294140031146965319L},
                        precision);
        }
}

void test()
{
        LOG("Test Madgwick filter");
        test_impl<float>(1e-6);
        test_impl<double>(1e-15);
        test_impl<long double>(0);
        LOG("Test Madgwick filter passed");
}

TEST_SMALL("Madgwick", test)
}
}
