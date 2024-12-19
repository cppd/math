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
#include <src/filter/attitude/ekf/constant.h>
#include <src/filter/attitude/ekf/matrix.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace ns::filter::attitude::ekf::test
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

template <std::size_t N, typename T>
void test_equal(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b, const T precision)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!equal(a[i], b[i], precision))
                {
                        error(to_string(a) + " is not equal to " + to_string(b));
                }
        }
}

template <std::size_t R, std::size_t C, typename T>
void test_equal(const numerical::Matrix<R, C, T>& a, const numerical::Matrix<R, C, T>& b, const T precision)
{
        for (std::size_t i = 0; i < R; ++i)
        {
                for (std::size_t j = 0; j < C; ++j)
                {
                        if (!equal(a[i, j], b[i, j], precision))
                        {
                                error(to_string(a) + " is not equal to " + to_string(b));
                        }
                }
        }
}

template <typename T>
void test_cross(const T precision)
{
        const numerical::Vector<3, T> v1{1, 2, 3};
        const numerical::Vector<3, T> v2{3, -2, 1};

        test_equal(cross(v1, v2), cross_matrix<1>(v1) * v2, precision);

        const numerical::Matrix<3, 3, T> m = cross_matrix<1>(v1);
        test_equal(m * m, cross_matrix<2>(v1), precision);
        test_equal(m * m * m, cross_matrix<3>(v1), precision);
        test_equal(m * m * m * m, cross_matrix<4>(v1), precision);
        test_equal(m * m * m * m * m, cross_matrix<5>(v1), precision);
        test_equal(m * m * m * m * m * m, cross_matrix<6>(v1), precision);
}

template <typename T>
void test_state(const T precision)
{
        using Matrix3 = numerical::Matrix<3, 3, T>;
        using Matrix6 = numerical::Matrix<6, 6, T>;

        constexpr numerical::Vector<3, T> W(W_THRESHOLD<T>, W_THRESHOLD<T> / T{2}, W_THRESHOLD<T> * T{2});

        {
                const Matrix3 m = state_transition_matrix_3(T{1000} * W, T{0.01});
                const Matrix3 c = {
                        {    0.999999978750000092988L,  0.000200002498249989085633L, -4.99899995625437641756e-05L},
                        {-0.000199997498250010964594L,     0.999999975000000109382L,  0.000100004999124978133608L},
                        { 5.00099995624562483812e-05L, -9.99949991250218915054e-05L,     0.999999993750000027359L}
                };
                test_equal(m, c, precision);
        }
        {
                const Matrix3 m = state_transition_matrix_3(W / T{10}, T{0.1});
                const Matrix3 c = {
                        {    0.999999999999978750017L,  2.00000002500000027457e-07L, -4.99999900000000068669e-08L},
                        {-1.99999997500000027466e-07L,     0.999999999999974999978L,  1.00000005000000013729e-07L},
                        {  5.0000010000000006864e-08L, -9.99999950000000137333e-08L,     0.999999999999993750008L}
                };
                test_equal(m, c, precision);
        }
        {
                const Matrix6 m = state_transition_matrix_6(T{1000} * W, T{0.01});
                const Matrix3 c00 = {
                        {    0.999999978750000092988L,  0.000200002498249989085633L, -4.99899995625437641756e-05L},
                        {-0.000199997498250010964594L,     0.999999975000000109382L,  0.000100004999124978133608L},
                        { 5.00099995624562483812e-05L, -9.99949991250218915054e-05L,     0.999999993750000027359L}
                };
                const Matrix3 c01 = {
                        { -0.00999999992916666706103L, -1.00000832895752153731e-06L,  2.49966665572806654057e-07L},
                        { 9.99991662290898605391e-07L,  -0.00999999991666666709319L, -5.00016664478727967538e-07L},
                        {-2.50033332239298381617e-07L,   4.9998333114548210381e-07L,  -0.00999999997916666692984L}
                };
                const Matrix3 c10 = {
                        {0, 0, 0},
                        {0, 0, 0},
                        {0, 0, 0}
                };
                const Matrix3 c11 = {
                        {1, 0, 0},
                        {0, 1, 0},
                        {0, 0, 1}
                };
                test_equal(numerical::block<0, 0, 3, 3>(m), c00, precision);
                test_equal(numerical::block<0, 3, 3, 3>(m), c01, precision);
                test_equal(numerical::block<3, 0, 3, 3>(m), c10, precision);
                test_equal(numerical::block<3, 3, 3, 3>(m), c11, precision);
        }
        {
                const Matrix6 m = state_transition_matrix_6(W / T{10}, T{0.1});
                const Matrix3 c00 = {
                        {    0.999999999999978750017L,  2.00000002500000027457e-07L, -4.99999900000000068669e-08L},
                        {-1.99999997500000027466e-07L,     0.999999999999974999978L,  1.00000005000000013729e-07L},
                        {  5.0000010000000006864e-08L, -9.99999950000000137333e-08L,     0.999999999999993750008L}
                };
                const Matrix3 c01 = {
                        {  -0.0999999999999992972147L, -1.00000000833333352612e-08L,  2.49999966666666714875e-09L},
                        { 9.99999991666666859524e-09L,   -0.0999999999999991722198L, -5.00000016666666763086e-09L},
                        {-2.50000033333333381535e-09L,  4.99999983333333429736e-09L,   -0.0999999999999997972149L}
                };
                const Matrix3 c10 = {
                        {0, 0, 0},
                        {0, 0, 0},
                        {0, 0, 0}
                };
                const Matrix3 c11 = {
                        {1, 0, 0},
                        {0, 1, 0},
                        {0, 0, 1}
                };
                test_equal(numerical::block<0, 0, 3, 3>(m), c00, precision);
                test_equal(numerical::block<0, 3, 3, 3>(m), c01, precision);
                test_equal(numerical::block<3, 0, 3, 3>(m), c10, precision);
                test_equal(numerical::block<3, 3, 3, 3>(m), c11, precision);
        }
}

template <typename T>
void test_impl(const T precision)
{
        test_cross<T>(precision);
        test_state<T>(precision);
}

void test()
{
        LOG("Test attitude EKF matrix");
        test_impl<float>(1e-5);
        test_impl<double>(1e-14);
        test_impl<long double>(0);
        LOG("Test attitude EKF matrix passed");
}

TEST_SMALL("Attitude EKF Matrix", test)
}
}
