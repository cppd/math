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
#include <src/com/random/pcg.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <type_traits>

namespace ns::numerical
{
namespace
{
template <typename T, bool JPL>
struct Test final
{
        static constexpr QuaternionHJ<T, JPL> A = QuaternionHJ<T, JPL>(2, {3, 4, 5});
        static constexpr QuaternionHJ<T, JPL> B = QuaternionHJ<T, JPL>(11, {12, 13, 14});

        static_assert(A == QuaternionHJ<T, JPL>(2, {3, 4, 5}));
        static_assert(A.w() == 2);
        static_assert(A.x() == 3);
        static_assert(A.y() == 4);
        static_assert(A.z() == 5);
        static_assert(A.vec() == Vector<3, T>(3, 4, 5));
        static_assert(A.conjugate() == QuaternionHJ<T, JPL>(2, {-3, -4, -5}));
        static_assert(A * T{3} == QuaternionHJ<T, JPL>(6, {9, 12, 15}));
        static_assert(T{3} * A == QuaternionHJ<T, JPL>(6, {9, 12, 15}));
        static_assert(A / T{2} == QuaternionHJ<T, JPL>(1, {T{3} / 2, 2, T{5} / 2}));
        static_assert(A + B == QuaternionHJ<T, JPL>(13, {15, 17, 19}));
        static_assert(A - B == QuaternionHJ<T, JPL>(-9, {-9, -9, -9}));
        static_assert(
                A * B == (JPL ? QuaternionHJ<T, JPL>(-136, {66, 52, 92}) : QuaternionHJ<T, JPL>(-136, {48, 88, 74})));
        static_assert(
                A * Vector<3, T>(11, 12, 13)
                == (JPL ? QuaternionHJ<T, JPL>(-146, {30, 8, 34}) : QuaternionHJ<T, JPL>(-146, {14, 40, 18})));
        static_assert(
                Vector<3, T>(11, 12, 13) * A
                == (JPL ? QuaternionHJ<T, JPL>(-146, {14, 40, 18}) : QuaternionHJ<T, JPL>(-146, {30, 8, 34})));
        static_assert(multiply_vec(A, B) == (JPL ? Vector<3, T>(66, 52, 92) : Vector<3, T>(48, 88, 74)));
};

#define TEMPLATE(T)                      \
        template struct Test<float, T>;  \
        template struct Test<double, T>; \
        template struct Test<long double, T>;
TEMPLATE(false)
TEMPLATE(true)

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

template <typename T, bool JPL, typename P>
bool equal(const QuaternionHJ<T, JPL>& a, const QuaternionHJ<T, JPL>& b, const P precision)
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
        if constexpr (requires { a.w(); })
        {
                if (a.w() == 0 && b.w() == 0)
                {
                        if (!(equal(a.vec(), b.vec(), precision) || equal(a.vec(), -b.vec(), precision)))
                        {
                                error(to_string(a) + " is not equal to " + to_string(b));
                        }
                        return;
                }
        }

        if (!equal(a, b, precision))
        {
                error(to_string(a) + " is not equal to " + to_string(b));
        }
}

template <typename T, bool JPL>
QuaternionHJ<T, JPL> random_rotation_quaternion(PCG& pcg)
{
        std::uniform_real_distribution<T> urd(-10, 10);
        const QuaternionHJ<T, JPL> q{
                std::abs(urd(pcg)),
                {urd(pcg), urd(pcg), urd(pcg)}
        };
        return q.normalized();
}

template <typename T, bool JPL>
void test_constant(const T precision)
{
        test_equal(
                QuaternionHJ<T, JPL>(2, {4, 3, 5}).normalized(),
                QuaternionHJ<T, JPL>(
                        0.272165526975908677584L,
                        {0.544331053951817355168L, 0.408248290463863016363L, 0.680413817439771693974L}),
                precision);

        test_equal(
                QuaternionHJ<T, JPL>(-2, {4, 3, 5}).normalized(),
                QuaternionHJ<T, JPL>(
                        0.272165526975908677584L,
                        {-0.544331053951817355168L, -0.408248290463863016363L, -0.680413817439771693974L}),
                precision);

        test_equal(
                QuaternionHJ<T, JPL>(3, {-7, 2, -8}).inversed(),
                QuaternionHJ<T, JPL>(
                        0.0238095238095238095235L,
                        {0.0555555555555555555548L, -0.0158730158730158730157L, 0.0634920634920634920626L}),
                precision);

        test_equal(
                QuaternionHJ<T, JPL>::rotation_quaternion({4, -5, 6}, T{2}),
                QuaternionHJ<T, JPL>(
                        0.540302305868139717414L,
                        {0.383578074011068530816L, -0.479472592513835663554L, 0.57536711101660279621L}),
                precision);
}

template <typename T, bool JPL>
void test_rotation(const T precision)
{
        {
                const QuaternionHJ<T, JPL> q = QuaternionHJ<T, JPL>::rotation_quaternion({1, 0, 0}, T{1} / 10);
                const Vector<3, T> v(0, 1, 0);
                const Vector<3, T> r(0, 0.995004165278025766135L, (JPL ? -1 : 1) * 0.0998334166468281523107L);
                test_equal(rotate_vector(q, v), r, precision);
                test_equal((q * v * q.conjugate()).vec(), r, precision);
        }
        {
                const QuaternionHJ<T, JPL> q = QuaternionHJ<T, JPL>::rotation_quaternion({0, 1, 0}, T{1} / 10);
                const Vector<3, T> v(1, 0, 0);
                const Vector<3, T> r(0.995004165278025766135L, 0, (JPL ? -1 : 1) * -0.0998334166468281523107L);
                test_equal(rotate_vector(q, v), r, precision);
                test_equal((q * v * q.conjugate()).vec(), r, precision);
        }
        {
                const QuaternionHJ<T, JPL> q = QuaternionHJ<T, JPL>::rotation_quaternion({0, 0, 1}, T{1} / 10);
                const Vector<3, T> v(1, 0, 0);
                const Vector<3, T> r(0.995004165278025766135L, (JPL ? -1 : 1) * 0.0998334166468281523107L, 0);
                test_equal(rotate_vector(q, v), r, precision);
                test_equal((q * v * q.conjugate()).vec(), r, precision);
        }
}

template <typename T, bool JPL>
void test_random(const T precision)
{
        {
                PCG pcg;
                std::uniform_real_distribution<T> urd(-100, 100);
                for (int i = 0; i < 100; ++i)
                {
                        const QuaternionHJ<T, JPL> q = random_rotation_quaternion<T, JPL>(pcg);
                        const Vector<3, T> v{urd(pcg), urd(pcg), urd(pcg)};
                        const Vector<3, T> r1 = rotate_vector(q, v);
                        const QuaternionHJ<T, JPL> q1 = q * v * q.conjugate();
                        const QuaternionHJ<T, JPL> q2 = q * QuaternionHJ<T, JPL>(0, v) * q.conjugate();
                        test_equal(r1, q1.vec(), precision);
                        test_equal(r1, q2.vec(), precision);
                        test_equal(T{0}, q1.w(), precision);
                        test_equal(T{0}, q2.w(), precision);
                }
        }

        {
                PCG pcg;
                std::uniform_real_distribution<T> urd(-100, 100);
                for (int i = 0; i < 100; ++i)
                {
                        const QuaternionHJ<T, JPL> q = random_rotation_quaternion<T, JPL>(pcg);
                        const Matrix<3, 3, T> m = unit_quaternion_to_rotation_matrix(q);
                        const Vector<3, T> v{urd(pcg), urd(pcg), urd(pcg)};
                        const Vector<3, T> r1 = rotate_vector(q, v);
                        const Vector<3, T> r2 = m * v;
                        test_equal(r1, r2, precision);
                }
        }

        {
                PCG pcg;
                for (int i = 0; i < 100; ++i)
                {
                        const QuaternionHJ<T, JPL> q1 = random_rotation_quaternion<T, JPL>(pcg);
                        const Matrix<3, 3, T> m = unit_quaternion_to_rotation_matrix(q1);
                        const QuaternionHJ<T, JPL> q2 = rotation_matrix_to_unit_quaternion<QuaternionHJ<T, JPL>>(m);
                        test_equal(q1, q2, precision);
                }
        }

        {
                const auto m = [](const auto& q)
                {
                        return unit_quaternion_to_rotation_matrix(q);
                };

                PCG pcg;
                for (int i = 0; i < 100; ++i)
                {
                        const QuaternionHJ<T, JPL> q1 = random_rotation_quaternion<T, JPL>(pcg);
                        const QuaternionHJ<T, JPL> q2 = random_rotation_quaternion<T, JPL>(pcg);
                        test_equal(m(q1 * q2), m(q1) * m(q2), precision);
                }
        }
}

template <typename T>
void test_vector_rotation(const T precision)
{
        test_equal(
                rotate_vector({-5, 6, 4}, T{2}, {3, -5, 2}),
                {5.46996008744151012305L, 0.27754662586912375613L, -2.82886982950179797927L}, precision);
}

template <typename T, bool JPL>
void test(const T precision)
{
        test_constant<T, JPL>(precision);
        test_rotation<T, JPL>(precision);
        test_random<T, JPL>(precision);
}

template <typename T>
void test(const T precision)
{
        test_vector_rotation<T>(precision);

        test<T, false>(precision);
        test<T, true>(precision);
}

void test_quaternion()
{
        LOG("Test quaternion");
        test<float>(1e-4);
        test<double>(1e-13);
        test<long double>(1e-16);
        LOG("Test quaternion passed");
}

TEST_SMALL("Quaternion", test_quaternion)
}
}
