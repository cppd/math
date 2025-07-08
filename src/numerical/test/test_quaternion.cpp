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

#include <src/com/constant.h>
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
#include <limits>
#include <random>
#include <tuple>
#include <type_traits>

namespace ns::numerical
{
namespace
{
template <typename T, bool JPL>
struct Test final
{
        using Quaternion = QuaternionHJ<T, JPL>;

        static constexpr Quaternion A = Quaternion(2, {3, 4, 5});
        static constexpr Quaternion B = Quaternion(11, {12, 13, 14});
        static constexpr Vector<3, T> V = Vector<3, T>(11, 12, 13);
        static constexpr T INF = std::numeric_limits<T>::infinity();

        static_assert(sizeof(Quaternion) == sizeof(QuaternionHJ<T, !JPL>));

        static_assert(A == Quaternion(2, {3, 4, 5}));
        static_assert(A == Quaternion(QuaternionHJ<T, !JPL>(A)));
        static_assert(A.w() == 2);
        static_assert(A.x() == 3);
        static_assert(A.y() == 4);
        static_assert(A.z() == 5);
        static_assert(QuaternionHJ<T, !JPL>(A).w() == 2);
        static_assert(QuaternionHJ<T, !JPL>(A).x() == 3);
        static_assert(QuaternionHJ<T, !JPL>(A).y() == 4);
        static_assert(QuaternionHJ<T, !JPL>(A).z() == 5);
        static_assert(!A.is_unit());
        static_assert(!B.is_unit());
        static_assert(!A.is_normalized());
        static_assert(!B.is_normalized());
        static_assert(is_finite(A));
        static_assert(is_finite(B));
        static_assert(A.vec() == Vector<3, T>(3, 4, 5));
        static_assert(A.conjugate() == Quaternion(2, {-3, -4, -5}));
        static_assert(A * T{3} == Quaternion(6, {9, 12, 15}));
        static_assert(T{3} * A == Quaternion(6, {9, 12, 15}));
        static_assert(A / T{2} == Quaternion(1, {T{3} / 2, 2, T{5} / 2}));
        static_assert(A + B == Quaternion(13, {15, 17, 19}));
        static_assert(A - B == Quaternion(-9, {-9, -9, -9}));

        static_assert(Quaternion(1, {0, 0, 0}).is_unit());
        static_assert(Quaternion(0, {1, 0, 0}).is_unit());
        static_assert(Quaternion(0, {0, 1, 0}).is_unit());
        static_assert(Quaternion(0, {0, 0, 1}).is_unit());

        static_assert(Quaternion(1, {0, 0, 0}).is_normalized());
        static_assert(Quaternion(0, {1, 0, 0}).is_normalized());
        static_assert(Quaternion(0, {0, 1, 0}).is_normalized());
        static_assert(Quaternion(0, {0, 0, 1}).is_normalized());

        static_assert(Quaternion(-1, {0, 0, 0}).is_unit());
        static_assert(!Quaternion(-1, {0, 0, 0}).is_normalized());

        static_assert(A * B == (JPL ? Quaternion(-136, {66, 52, 92}) : Quaternion(-136, {48, 88, 74})));
        static_assert(B * A == (!JPL ? Quaternion(-136, {66, 52, 92}) : Quaternion(-136, {48, 88, 74})));
        static_assert(A * V == (JPL ? Quaternion(-146, {30, 8, 34}) : Quaternion(-146, {14, 40, 18})));
        static_assert(V * A == (!JPL ? Quaternion(-146, {30, 8, 34}) : Quaternion(-146, {14, 40, 18})));
        static_assert(multiply_vec(A, B) == (JPL ? Vector<3, T>(66, 52, 92) : Vector<3, T>(48, 88, 74)));
        static_assert(multiply_vec(B, A) == (!JPL ? Vector<3, T>(66, 52, 92) : Vector<3, T>(48, 88, 74)));

        static_assert(!is_finite(Quaternion(-INF, {-INF, -INF, -INF})));
        static_assert(!is_finite(Quaternion(INF, {1, 1, 1})));
        static_assert(!is_finite(Quaternion(1, {INF, 1, 1})));
        static_assert(!is_finite(Quaternion(1, {1, INF, 1})));
        static_assert(!is_finite(Quaternion(1, {1, 1, INF})));
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

template <typename T>
void test_normalized(const T& v)
{
        if (!(v.is_unit() && v.is_normalized()))
        {
                error(to_string(v) + " is not normalized");
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

template <typename T>
std::tuple<T, Vector<3, T>> random_rotation_vector(PCG& pcg)
{
        std::uniform_real_distribution<T> urd(-10, 10);
        std::uniform_real_distribution<T> urd_angle(-3 * PI<T>, 3 * PI<T>);
        const Vector<3, T> v(urd(pcg), urd(pcg), urd(pcg));
        return {urd_angle(pcg), v.normalized()};
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
                QuaternionHJ<T, JPL>::rotation_quaternion(2, Vector<3, T>(4, -5, 6)),
                QuaternionHJ<T, JPL>(
                        0.540302305868139717414L,
                        {0.383578074011068530816L, -0.479472592513835663554L, 0.57536711101660279621L}),
                precision);

        test_equal(
                QuaternionHJ<T, JPL>::rotation_quaternion(T{1.1L} * PI<T>, Vector<3, T>(-4, 5, -3)),
                QuaternionHJ<T, JPL>(
                        0.156434465040230869204L,
                        {0.558720898666968221263L, -0.698401123333710276565L, 0.419040674000226165934L}),
                precision);

        {
                const QuaternionHJ<T, JPL> q = QuaternionHJ<T, JPL>(2, {4, 3, 5});
                if (q.is_unit() || q.is_normalized())
                {
                        error(to_string(q) + " is unit or normalized");
                }
                const QuaternionHJ<T, JPL> qn = q.normalized();
                if (!qn.is_unit() || !qn.is_normalized())
                {
                        error(to_string(q) + " is not unit or not normalized");
                }
        }
}

template <typename T, bool JPL>
void test_rotation(const T precision)
{
        {
                const QuaternionHJ<T, JPL> q = QuaternionHJ<T, JPL>::rotation_quaternion(T{1} / 10, {1, 0, 0});
                const Vector<3, T> v(0, 1, 0);
                const Vector<3, T> r(0, 0.995004165278025766135L, (JPL ? -1 : 1) * 0.0998334166468281523107L);
                test_equal(rotate_vector(q, v), r, precision);
                test_equal((q * v * q.conjugate()).vec(), r, precision);
        }
        {
                const QuaternionHJ<T, JPL> q = QuaternionHJ<T, JPL>::rotation_quaternion(T{1} / 10, {0, 1, 0});
                const Vector<3, T> v(1, 0, 0);
                const Vector<3, T> r(0.995004165278025766135L, 0, (JPL ? -1 : 1) * -0.0998334166468281523107L);
                test_equal(rotate_vector(q, v), r, precision);
                test_equal((q * v * q.conjugate()).vec(), r, precision);
        }
        {
                const QuaternionHJ<T, JPL> q = QuaternionHJ<T, JPL>::rotation_quaternion(T{1} / 10, {0, 0, 1});
                const Vector<3, T> v(1, 0, 0);
                const Vector<3, T> r(0.995004165278025766135L, (JPL ? -1 : 1) * 0.0998334166468281523107L, 0);
                test_equal(rotate_vector(q, v), r, precision);
                test_equal((q * v * q.conjugate()).vec(), r, precision);
        }
        {
                const QuaternionHJ<T, JPL> q = QuaternionHJ<T, JPL>(-2, {5.2, -3.3, 4.4}).normalized();
                const numerical::Vector<3, T> v = rotate_vector(q, {2.1, -3.2, 4.3});
                const numerical::Vector<3, T> c =
                        JPL ? numerical::Vector<3, T>(
                                      5.0222059063468757101L, -2.42440854951868102712L, 1.4281775167237726246L)
                            : numerical::Vector<3, T>(
                                      5.03656387665198215136L, -0.711894273127752929284L, 2.69559471365638818612L);
                test_equal(v, c, precision);
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
                        const Matrix<3, 3, T> m = q.rotation_matrix();
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
                        const QuaternionHJ<T, JPL> q2 = QuaternionHJ<T, JPL>::rotation_quaternion(q1.rotation_matrix());
                        test_equal(q1, q2, precision);
                }
        }

        {
                const auto m = [](const auto& q)
                {
                        return q.rotation_matrix();
                };

                PCG pcg;
                for (int i = 0; i < 100; ++i)
                {
                        const QuaternionHJ<T, JPL> q1 = random_rotation_quaternion<T, JPL>(pcg);
                        const QuaternionHJ<T, JPL> q2 = random_rotation_quaternion<T, JPL>(pcg);
                        test_equal(m(q1 * q2), m(q1) * m(q2), precision);
                }
        }

        {
                PCG pcg;
                for (int i = 0; i < 100; ++i)
                {
                        const auto [angle, axis] = random_rotation_vector<T>(pcg);
                        const QuaternionHJ<T, JPL> q1 = QuaternionHJ<T, JPL>::rotation_quaternion(angle, axis);
                        const QuaternionHJ<T, JPL> q2 =
                                rotation_vector_to_quaternion<QuaternionHJ<T, JPL>>(angle, axis);
                        const Matrix<3, 3, T> m = rotation_vector_to_matrix<JPL>(angle, axis);
                        test_normalized(q1);
                        test_normalized(q2);
                        test_equal(q1, q2, precision);
                        test_equal(q1.rotation_matrix(), m, precision);
                }
        }
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
