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

#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/math.h>
#include <src/com/print.h>
#include <src/com/random/pcg.h>
#include <src/com/type/name.h>
#include <src/numerical/identity.h>
#include <src/numerical/matrix.h>
#include <src/numerical/solve.h>
#include <src/numerical/vector.h>
#include <src/test/test.h>

#include <array>
#include <cstddef>
#include <random>
#include <type_traits>
#include <vector>

namespace ns::numerical
{
namespace
{
template <typename T>
void write(T&& /*v*/)
{
        // LOG(std::forward<T>(v));
}

template <typename T>
constexpr std::array MATRIX = std::to_array<Vector<4, T>>({
        { 2,  2,  3,  4},
        { 5, 12,  7,  8},
        { 9, 10, 22, 12},
        {13, 14, 15, 32}
});

template <typename T>
constexpr std::array INVERSE = std::to_array<Vector<4, T>>({
        {   T{99} / 10,    T{1} / 10,  T{-7} / 10,     T{-1}},
        {  T{-61} / 50,    T{3} / 25,   T{3} / 50, T{1} / 10},
        { T{-107} / 50,   T{-3} / 50,  T{11} / 50,  T{1} / 5},
        {T{-497} / 200, T{-13} / 200, T{31} / 200, T{3} / 10}
});

template <typename T>
constexpr Vector<4, T> VECTOR{1, 2, 3, 4};

template <typename T>
constexpr Vector<4, T> SOLVED{T{4}, T{-2} / 5, T{-4} / 5, T{-19} / 20};

template <typename T>
constexpr bool are_equal(const T& a, const T& b, const T& abs_precision, const T& rel_precision)
{
        if (a == b)
        {
                return true;
        }
        const T abs = absolute(a - b);
        if ((a == 0 || b == 0) && (abs <= abs_precision))
        {
                return true;
        }
        const T rel = abs / std::max(absolute(a), absolute(b));
        return rel <= rel_precision;
}

template <std::size_t N, typename T>
constexpr bool are_equal(const Vector<N, T>& a, const Vector<N, T>& b, const T& abs_precision, const T& rel_precision)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!are_equal(a[i], b[i], abs_precision, rel_precision))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t R, std::size_t C, typename T>
constexpr bool are_equal(
        const std::array<Vector<C, T>, R>& a,
        const std::array<Vector<C, T>, R>& b,
        const T& abs_precision,
        const T& rel_precision)
{
        for (std::size_t i = 0; i < R; ++i)
        {
                if (!are_equal(a[i], b[i], abs_precision, rel_precision))
                {
                        return false;
                }
        }
        return true;
}

template <typename T>
constexpr bool test_solve(const std::type_identity_t<T>& abs_precision, const std::type_identity_t<T>& rel_precision)
{
        return are_equal(SOLVED<T>, linear_solve(MATRIX<T>, VECTOR<T>), abs_precision, rel_precision)
               && are_equal(INVERSE<T>, inverse(MATRIX<T>), abs_precision, rel_precision);
}

static_assert(test_solve<float>(0, 6e-7));
static_assert(test_solve<double>(0, 2e-15));
static_assert(test_solve<long double>(0, 7e-19));
static_assert(test_solve<__float128>(0, 2e-33));

//

template <std::size_t R, std::size_t C, typename T>
bool are_equal(const Matrix<R, C, T>& a, const Matrix<R, C, T>& b, const T& abs_precision, const T& rel_precision)
{
        for (std::size_t r = 0; r < R; ++r)
        {
                for (std::size_t c = 0; c < C; ++c)
                {
                        if (!are_equal(a(r, c), b(r, c), abs_precision, rel_precision))
                        {
                                return false;
                        }
                }
        }
        return true;
}

template <std::size_t ROWS, std::size_t COLUMNS, typename T>
std::vector<std::array<Vector<COLUMNS, T>, ROWS>> random_matrices(const int count)
{
        PCG engine;
        std::uniform_real_distribution<T> urd(-10, 10);
        std::vector<std::array<Vector<COLUMNS, T>, ROWS>> res(count);
        for (int i = 0; i < count; ++i)
        {
                for (std::size_t r = 0; r < ROWS; ++r)
                {
                        for (std::size_t c = 0; c < COLUMNS; ++c)
                        {
                                res[i][r][c] = urd(engine);
                        }
                }
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<Vector<N, T>> random_vectors(const int count)
{
        PCG engine;
        std::uniform_real_distribution<T> urd(-10, 10);
        std::vector<Vector<N, T>> res(count);
        for (int i = 0; i < count; ++i)
        {
                for (std::size_t n = 0; n < N; ++n)
                {
                        res[i][n] = urd(engine);
                }
        }
        return res;
}

template <std::size_t N, typename T>
void test_solve_vector(
        const int count,
        const std::type_identity_t<T>& abs_precision,
        const std::type_identity_t<T>& rel_precision)
{
        write("Test solve <" + to_string(N) + ", " + type_name<T>() + ">");

        const std::vector<std::array<Vector<N, T>, N>> matrices = random_matrices<N, N, T>(count);
        const std::vector<Vector<N, T>> vectors = random_vectors<N, T>(count);

        const std::vector<Vector<N, T>> solved = [&]
        {
                std::vector<Vector<N, T>> res(count);
                const Clock::time_point start_time = Clock::now();
                for (int i = 0; i < count; ++i)
                {
                        res[i] = linear_solve(matrices[i], vectors[i]);
                }
                write("Time = " + to_string_fixed(duration_from(start_time), 5) + " s");
                return res;
        }();

        for (int i = 0; i < count; ++i)
        {
                const Vector<N, T> multiplied = Matrix(matrices[i]) * solved[i];
                if (!are_equal(multiplied, vectors[i], abs_precision, rel_precision))
                {
                        error("Failed to solve:\nmatrix\n" + to_string(Matrix(matrices[i]))
                              + "\nsolved = " + to_string(solved[i]) + "\nvector = " + to_string(vectors[i])
                              + "\nmultiplied = " + to_string(multiplied));
                }
        }
}

template <std::size_t N, typename T>
void test_solve_inverse(
        const int count,
        const std::type_identity_t<T>& abs_precision,
        const std::type_identity_t<T>& rel_precision)
{
        write("Test inverse <" + to_string(N) + ", " + type_name<T>() + ">");

        const std::vector<std::array<Vector<N, T>, N>> matrices = random_matrices<N, N, T>(count);

        const std::vector<std::array<Vector<N, T>, N>> inversed = [&]
        {
                std::vector<std::array<Vector<N, T>, N>> res(count);
                const Clock::time_point start_time = Clock::now();
                for (int i = 0; i < count; ++i)
                {
                        res[i] = inverse(matrices[i]);
                }
                write("Time = " + to_string_fixed(duration_from(start_time), 5) + " s");
                return res;
        }();

        const Matrix<N, N, T> identity(IDENTITY_ARRAY<N, T>);

        for (int i = 0; i < count; ++i)
        {
                const Matrix<N, N, T> multiplied = Matrix(matrices[i]) * Matrix(inversed[i]);
                if (!are_equal(multiplied, identity, abs_precision, rel_precision))
                {
                        error("Failed to inverse:\nmatrix\n" + to_string(matrices[i]) + "\ninverse\n"
                              + to_string(inversed[i]) + "\nmultiplied\n" + to_string(multiplied));
                }
        }
}

template <std::size_t N, typename T>
void test_solve(
        const int count,
        const std::type_identity_t<T>& abs_precision,
        const std::type_identity_t<T>& rel_precision)
{
        test_solve_vector<N, T>(count, abs_precision, rel_precision);
        test_solve_inverse<N, T>(count, abs_precision, rel_precision);
}

template <typename T>
void test_solve(
        const int count,
        const std::type_identity_t<T>& abs_precision,
        const std::type_identity_t<T>& rel_precision)
{
        test_solve<1, T>(count, abs_precision, rel_precision);
        test_solve<2, T>(count, abs_precision, rel_precision);
        test_solve<3, T>(count, abs_precision, rel_precision);
        test_solve<4, T>(count, abs_precision, rel_precision);
        test_solve<5, T>(count, abs_precision, rel_precision);
        test_solve<6, T>(count, abs_precision, rel_precision);
        test_solve<7, T>(count, abs_precision, rel_precision);
        test_solve<8, T>(count, abs_precision, rel_precision);
}

void test()
{
        LOG("Test linear solve");
        test_solve<double>(1000, 1e-8, 1e-6);
        test_solve<long double>(200, 1e-11, 1e-9);
        LOG("Test linear solve passed");
}

TEST_SMALL("Linear Solve", test)
}
}
