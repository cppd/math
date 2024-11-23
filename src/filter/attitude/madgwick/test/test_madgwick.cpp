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
        cmp(0.636766707106268591585L, 0.193224325555701386569L, 0.638162672093604048059L, 0.387222061201237693467L);

        q = m.update({0.01L, 0.02L, 0.03L}, {0, 0, 0}, {15, -20, 25}, beta, zeta, 0.1L);
        cmp(0.629098656826129842108L, 0.195344857404973504277L, 0.642967426834155368468L, 0.39072772450295224474L);

        q = m.update({0.01L, 0.02L, 0.03L}, {3, 5, 8}, {0, 0, 0}, beta, zeta, 0.1L);
        cmp(0.633398015523262782164L, 0.209550663614859412445L, 0.611087945333374136082L, 0.425989432237467293069L);

        q = m.update({0.01L, 0.02L, 0.03L}, {0, 0, 0}, {0, 0, 0}, beta, zeta, 0.1L);
        cmp(0.625683359054655725666L, 0.211207042892938397867L, 0.615871054823818319898L, 0.429668201132805046807L);
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
