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
void test_cross_matrix(const T precision)
{
        using Vector3 = numerical::Vector<3, T>;
        using Matrix3 = numerical::Matrix<3, 3, T>;

        {
                const Vector3 v(1, 2, 3);
                const Matrix3 m = cross_matrix<1>(v);
                const Matrix3 c = {
                        { 0, -3,  2},
                        { 3,  0, -1},
                        {-2,  1,  0}
                };
                test_equal(m, c, precision);
        }

        const Vector3 v1(1, 2, 3);
        const Vector3 v2(3, -2, 1);

        test_equal(cross(v1, v2), cross_matrix<1>(v1) * v2, precision);

        const Matrix3 m = cross_matrix<1>(v1);
        test_equal(m * m, cross_matrix<2>(v1), precision);
        test_equal(m * m * m, cross_matrix<3>(v1), precision);
        test_equal(m * m * m * m, cross_matrix<4>(v1), precision);
        test_equal(m * m * m * m * m, cross_matrix<5>(v1), precision);
        test_equal(m * m * m * m * m * m, cross_matrix<6>(v1), precision);
}

template <typename T>
void test_state_transition_matrix(const T precision)
{
        using Vector3 = numerical::Vector<3, T>;
        using Matrix3 = numerical::Matrix<3, 3, T>;
        using Matrix6 = numerical::Matrix<6, 6, T>;

        constexpr Vector3 W(W_THRESHOLD<T>, W_THRESHOLD<T> / T{2}, W_THRESHOLD<T> * T{2});

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
void test_noise_covariance_matrix(const T precision)
{
        using Vector3 = numerical::Vector<3, T>;
        using Matrix3 = numerical::Matrix<3, 3, T>;
        using Matrix6 = numerical::Matrix<6, 6, T>;

        constexpr Vector3 W(W_THRESHOLD<T>, W_THRESHOLD<T> / T{2}, W_THRESHOLD<T> * T{2});

        {
                const Matrix3 m = noise_covariance_matrix_3(T{0.01}, T{0.01});
                const Matrix3 c = {
                        {0.000100000000000000004164L,                           0,                           0},
                        {                          0, 0.000100000000000000004164L,                           0},
                        {                          0,                           0, 0.000100000000000000004164L}
                };
                test_equal(m, c, precision);
        }
        {
                const Matrix6 m = noise_covariance_matrix_6(T{10000} * W, T{0.01}, T{0.001}, T{0.01});
                const Matrix3 c00 = {
                        {0.000100000333333262504191L, 8.33333041391428018334e-18L, 3.33333216556571207334e-17L},
                        {8.33333041391428018334e-18L, 0.000100000333333250004195L, 1.66666608278285603667e-17L},
                        {3.33333216556571207334e-17L, 1.66666608278285603667e-17L, 0.000100000333333312504169L}
                };
                const Matrix3 c01 = {
                        {-4.99999822916705543092e-08L, -3.33354079162119080499e-11L,  8.32499781432857037213e-12L},
                        {    3.33312412504592461e-11L, -4.99999791666712398148e-08L, -1.66708289574204504858e-11L},
                        {-8.34166447733921816457e-12L,  1.66624956259151265892e-11L, -4.99999947916678122965e-08L}
                };
                const Matrix3 c10 = c01.transposed();
                const Matrix3 c11 = {
                        {1.0000000000000000416e-05L,                          0,                          0},
                        {                         0, 1.0000000000000000416e-05L,                          0},
                        {                         0,                          0, 1.0000000000000000416e-05L}
                };
                test_equal(numerical::block<0, 0, 3, 3>(m), c00, precision);
                test_equal(numerical::block<0, 3, 3, 3>(m), c01, precision);
                test_equal(numerical::block<3, 0, 3, 3>(m), c10, precision);
                test_equal(numerical::block<3, 3, 3, 3>(m), c11, precision);
        }
        {
                const Matrix6 m = noise_covariance_matrix_6(W / T{10}, T{0.01}, T{0.001}, T{0.01});
                const Matrix3 c00 = {
                        {  0.0001000003333333333375L, 8.33333333333333573797e-28L, 3.33333333333333429519e-27L},
                        {8.33333333333333573797e-28L,   0.0001000003333333333375L, 1.66666666666666714759e-27L},
                        {3.33333333333333429519e-27L, 1.66666666666666714759e-27L,   0.0001000003333333333375L}
                };
                const Matrix3 c01 = {
                        {-5.00000000000000013531e-08L, -3.33333333541666721733e-16L,  8.33333325000000137635e-17L},
                        { 3.33333333125000055045e-16L, -5.00000000000000010397e-08L, -1.66666667083333360858e-16L},
                        { -8.3333334166666680425e-17L,  1.66666666250000027531e-16L, -5.00000000000000026036e-08L}
                };
                const Matrix3 c10 = c01.transposed();
                const Matrix3 c11 = {
                        {1.0000000000000000416e-05L,                          0,                          0},
                        {                         0, 1.0000000000000000416e-05L,                          0},
                        {                         0,                          0, 1.0000000000000000416e-05L}
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
        test_cross_matrix<T>(precision);
        test_state_transition_matrix<T>(precision);
        test_noise_covariance_matrix<T>(precision);
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
