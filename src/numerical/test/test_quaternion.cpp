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

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>
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
struct Test final
{
        static constexpr Quaternion<T> A{1, 2, 3, 4};
        static constexpr Quaternion<T> B{11, 12, 13, 14};

        static_assert(A == Quaternion<T>(1, {2, 3, 4}));
        static_assert(A == Quaternion<T>(Vector<4, T>(1, 2, 3, 4)));
        static_assert(A[0] == 1);
        static_assert(A[1] == 2);
        static_assert(A[2] == 3);
        static_assert(A[3] == 4);
        static_assert(A.data() == Vector<4, T>(1, 2, 3, 4));
        static_assert(A.imag() == Vector<3, T>(2, 3, 4));
        static_assert(A.conjugate() == Quaternion<T>(1, -2, -3, -4));
        static_assert(A * T{3} == Quaternion<T>(3, 6, 9, 12));
        static_assert(T{3} * A == Quaternion<T>(3, 6, 9, 12));
        static_assert(A / T{2} == Quaternion<T>(T{1} / 2, 1, T{3} / 2, 2));
        static_assert(A + B == Quaternion<T>(12, 14, 16, 18));
        static_assert(A - B == Quaternion<T>(-10, -10, -10, -10));
        static_assert(A * B == Quaternion<T>(-108, 24, 66, 48));
        static_assert(A * Vector<3, T>(11, 12, 13) == Quaternion<T>(-110, 2, 30, 4));
};

template struct Test<float>;
template struct Test<double>;
template struct Test<long double>;

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

template <typename T>
void test_equal(const Quaternion<T>& a, const Quaternion<T>& b, const T precision)
{
        for (std::size_t i = 0; i < 4; ++i)
        {
                if (!equal(a[i], b[i], precision))
                {
                        error(to_string(a) + " is not equal to " + to_string(b));
                }
        }
}

template <typename T>
void test(const T precision)
{
        {
                Quaternion<T> q(2, 3, 4, 5);
                q.normalize();
                test_equal(
                        q,
                        Quaternion<T>(
                                0.272165526975908677584L, 0.408248290463863016363L, 0.544331053951817355168L,
                                0.680413817439771693974L),
                        precision);
        }

        test_equal(
                Quaternion<T>(2, 4, 3, 5).normalized(),
                Quaternion<T>(
                        0.272165526975908677584L, 0.544331053951817355168L, 0.408248290463863016363L,
                        0.680413817439771693974L),
                precision);

        test_equal(
                Quaternion<T>(3, -7, 2, -8).inversed(),
                Quaternion<T>(
                        0.0238095238095238095235L, 0.0555555555555555555548L, -0.0158730158730158730157L,
                        0.0634920634920634920626L),
                precision);
}

void test_quaternion()
{
        LOG("Test quaternion");
        test<float>(1e-7);
        test<double>(0);
        test<long double>(0);
        LOG("Test quaternion passed");
}

TEST_SMALL("Quaternion", test_quaternion)
}
}
