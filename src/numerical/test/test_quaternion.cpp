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
#include <src/numerical/matrix.h>
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
        static constexpr Quaternion<T> A{2, 3, 4, 5};
        static constexpr Quaternion<T> B{11, 12, 13, 14};

        static_assert(A == Quaternion<T>(2, {3, 4, 5}));
        static_assert(A == Quaternion<T>(Vector<4, T>(2, 3, 4, 5)));
        static_assert(A[0] == 2);
        static_assert(A[1] == 3);
        static_assert(A[2] == 4);
        static_assert(A[3] == 5);
        static_assert(A.coeffs() == Vector<4, T>(2, 3, 4, 5));
        static_assert(A.vec() == Vector<3, T>(3, 4, 5));
        static_assert(A.conjugate() == Quaternion<T>(2, -3, -4, -5));
        static_assert(A * T{3} == Quaternion<T>(6, 9, 12, 15));
        static_assert(T{3} * A == Quaternion<T>(6, 9, 12, 15));
        static_assert(A / T{2} == Quaternion<T>(1, T{3} / 2, 2, T{5} / 2));
        static_assert(A + B == Quaternion<T>(13, 15, 17, 19));
        static_assert(A - B == Quaternion<T>(-9, -9, -9, -9));
        static_assert(A * B == Quaternion<T>(-136, 48, 88, 74));
        static_assert(A * Vector<3, T>(11, 12, 13) == Quaternion<T>(-146, 14, 40, 18));
        static_assert(multiply_vec(A, B) == Vector<3, T>(48, 88, 74));
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
void test(const T precision)
{
        {
                Quaternion<T> q(2, 3, 4, 5);
                q.normalize();
                test_equal(
                        q,
                        {0.272165526975908677584L, 0.408248290463863016363L, 0.544331053951817355168L,
                         0.680413817439771693974L},
                        precision);
        }

        test_equal(
                Quaternion<T>(2, 4, 3, 5).normalized(),
                {0.272165526975908677584L, 0.544331053951817355168L, 0.408248290463863016363L,
                 0.680413817439771693974L},
                precision);

        test_equal(
                Quaternion<T>(3, -7, 2, -8).inversed(),
                {0.0238095238095238095235L, 0.0555555555555555555548L, -0.0158730158730158730157L,
                 0.0634920634920634920626L},
                precision);

        test_equal(
                unit_quaternion_for_rotation({4, -5, 6}, T{2}),
                {0.540302305868139717414L, 0.383578074011068530816L, -0.479472592513835663554L,
                 0.57536711101660279621L},
                precision);

        test_equal(
                rotate_vector({-5, 6, 4}, T{2}, {3, -5, 2}),
                {5.46996008744151012305L, 0.27754662586912375613L, -2.82886982950179797927L}, precision);

        {
                const Quaternion<T> q1 = Quaternion<T>(2, -3, 4, -5).normalized();
                const Matrix<3, 3, T> m = unit_quaternion_to_rotation_matrix(q1);
                const Quaternion<T> q2 = rotation_matrix_to_unit_quaternion(m);
                test_equal(q1, q2, precision);
        }

        {
                const Quaternion<T> q = Quaternion<T>(2, 3, -4, 5).normalized();
                const Matrix<3, 3, T> m = unit_quaternion_to_rotation_matrix(q);
                const Vector<3, T> v{-3, 4, -5};
                const Vector<3, T> r1 = rotate_vector(q, v);
                const Vector<3, T> r2 = m * v;
                test_equal(r1, r2, precision);
        }
}

void test_quaternion()
{
        LOG("Test quaternion");
        test<float>(1e-6);
        test<double>(1e-15);
        test<long double>(1e-18);
        LOG("Test quaternion passed");
}

TEST_SMALL("Quaternion", test_quaternion)
}
}
