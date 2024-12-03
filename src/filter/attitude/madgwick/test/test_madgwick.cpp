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
#include <src/filter/attitude/madgwick/madgwick.h>
#include <src/numerical/quaternion.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace ns::filter::attitude::madgwick::test
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

template <typename T, typename P>
bool equal(const numerical::Quaternion<T>& a, const numerical::Quaternion<T>& b, const P precision)
{
        if (!equal(a.w(), b.w(), precision))
        {
                return false;
        }
        if (!equal(a.vec(), b.vec(), precision))
        {
                return false;
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
                q = m.update({0.01L, 0.02L, 0.03L}, {3, 5, 8}, beta, 0.1L);
        }
        cmp(0.949205178592176475989L, 0.272454412336692847847L, -0.156441973403004307483L, 0.0174364873135447197984L);

        q = m.update({0.01L, 0.02L, 0.03L}, {0, 0, 0}, beta, 0.1L);
        cmp(0.949197577534133588343L, 0.272676438295221403328L, -0.155892458787696229369L, 0.0192109368613610888279L);
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
                q = m.update({0.01L, 0.02L, 0.03L}, {3, 5, 8}, {15, -20, 25}, beta, zeta, 0.1L);
        }
        cmp(0.73596857429125746102L, 0.321315325862648540215L, 0.0360219744102775187024L, 0.594818574340999493702L);

        q = m.update({0.01L, 0.02L, 0.03L}, {0, 0, 0}, {15, -20, 25}, beta, zeta, 0.1L);
        cmp(0.730012933885808681272L, 0.323803884794340809022L, 0.0365891600948823576514L, 0.600744033607557567926L);

        q = m.update({0.01L, 0.02L, 0.03L}, {3, 5, 8}, {0, 0, 0}, beta, zeta, 0.1L);
        cmp(0.747328833805681051001L, 0.257295768854392600263L, 0.068847469272653983815L, 0.608735186650992511365L);

        q = m.update({0.01L, 0.02L, 0.03L}, {0, 0, 0}, {0, 0, 0}, beta, zeta, 0.1L);
        cmp(0.741477097724166598302L, 0.260072453732426620503L, 0.0699792752573717451365L, 0.614554255859150738576L);
}

template <typename T>
void test_impl(const T precision)
{
        test_m<T>(precision);
        test_mm<T>(precision);
}

void test()
{
        LOG("Test attitude Madgwick");
        test_impl<float>(1e-5);
        test_impl<double>(1e-14);
        test_impl<long double>(0);
        LOG("Test attitude Madgwick passed");
}

TEST_SMALL("Attitude Madgwick", test)
}
}
