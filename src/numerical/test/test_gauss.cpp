/*
Copyright (C) 2017-2021 Topological Manifold

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

#include "../gauss.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/time.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <random>

namespace ns::numerical
{
namespace
{
template <typename T>
void write(T&& /*v*/)
{
        //LOG(std::forward<T>(v));
}

// clang-format off
template <typename T>
constexpr std::array MATRIX = std::to_array<Vector<4, T>>
({
{2, 2, 3, 4},
{5, 12, 7, 8},
{9, 10, 22, 12},
{13, 14, 15, 32}
});
template <typename T>
constexpr std::array INVERSE = std::to_array<Vector<4, T>>
({
{T(99) / 10, T(1) / 10, T(-7) / 10, T(-1)},
{T(-61) / 50, T(3) / 25, T(3) / 50, T(1) / 10},
{T(-107) / 50, T(-3) / 50, T(11) / 50, T(1) / 5},
{T(-497) / 200, T(-13) / 200, T(31) / 200, T(3) / 10}
});
template <typename T>
constexpr std::array IDENTITY = std::to_array<Vector<4, T>>
({
{1, 0, 0, 0},
{0, 1, 0, 0},
{0, 0, 1, 0},
{0, 0, 0, 1}
});
// clang-format on
template <typename T>
constexpr Vector<4, T> ROW{1, 2, 3, 4};
template <typename T>
constexpr Vector<4, T> SOLVED{T(4), T(-2) / 5, T(-4) / 5, T(-19) / 20};

template <typename T>
constexpr T absolute(const T& v)
{
        return v < 0 ? -v : v;
}

template <typename T>
constexpr bool are_equal(const T& a, const T& b, const T& precision)
{
        if (a == b)
        {
                return true;
        }
        T rel = absolute(a - b) / std::max(absolute(a), absolute(b));
        return (rel < precision);
}

template <std::size_t N, typename T>
constexpr bool are_equal(const Vector<N, T>& a, const Vector<N, T>& b, const T& precision)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!are_equal(a[i], b[i], precision))
                {
                        return false;
                }
        }
        return true;
}

template <std::size_t R, std::size_t C, typename T>
constexpr bool are_equal(const std::array<Vector<C, T>, R>& a, const std::array<Vector<C, T>, R>& b, const T& precision)
{
        for (std::size_t i = 0; i < R; ++i)
        {
                if (!are_equal(a[i], b[i], precision))
                {
                        return false;
                }
        }
        return true;
}

template <typename T>
constexpr bool test_solve(const T& precision)
{
        if (!are_equal(INVERSE<T>, solve_gauss(MATRIX<T>, IDENTITY<T>), precision))
        {
                return false;
        }
        if (!are_equal(SOLVED<T>, solve_gauss(MATRIX<T>, ROW<T>), precision))
        {
                return false;
        }
        return true;
}

static_assert(test_solve<float>(6e-7));
static_assert(test_solve<double>(2e-15));
static_assert(test_solve<long double>(7e-19));
static_assert(test_solve<__float128>(2e-33));

//

template <std::size_t Rows, std::size_t Columns, typename T>
std::vector<std::array<Vector<Columns, T>, Rows>> random_matrices(const int count)
{
        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();
        std::uniform_real_distribution<T> urd(-10, 10);
        std::vector<std::array<Vector<Columns, T>, Rows>> res(count);
        for (int i = 0; i < count; ++i)
        {
                for (std::size_t r = 0; r < Rows; ++r)
                {
                        for (std::size_t c = 0; c < Columns; ++c)
                        {
                                res[i][r][c] = urd(random_engine);
                        }
                }
        }
        return res;
}

template <std::size_t N, typename T>
std::vector<Vector<N, T>> random_vectors(const int count)
{
        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();
        std::uniform_real_distribution<T> urd(-10, 10);
        std::vector<Vector<N, T>> res(count);
        for (int i = 0; i < count; ++i)
        {
                for (std::size_t n = 0; n < N; ++n)
                {
                        res[i][n] = urd(random_engine);
                }
        }
        return res;
}

template <std::size_t N, typename T>
void test_solve_vector(const int count, const std::type_identity_t<T>& precision)
{
        write("Test solve (" + to_string(N) + "), " + type_name<T>());

        const std::vector<std::array<Vector<N, T>, N>> matrices = random_matrices<N, N, T>(count);
        const std::vector<Vector<N, T>> vectors = random_vectors<N, T>(count);

        const std::vector<Vector<N, T>> solved = [&]
        {
                std::vector<Vector<N, T>> res(count);
                const TimePoint start_time = time();
                for (int i = 0; i < count; ++i)
                {
                        res[i] = solve_gauss(matrices[i], vectors[i]);
                }
                write("Time = " + to_string_fixed(duration_from(start_time), 5) + " s");
                return res;
        }();

        for (int i = 0; i < count; ++i)
        {
                Vector<N, T> multiplied;
                for (std::size_t r = 0; r < N; ++r)
                {
                        T sum = 0;
                        for (std::size_t s = 0; s < N; ++s)
                        {
                                sum += matrices[i][r][s] * solved[i][s];
                        }
                        multiplied[r] = sum;
                }
                if (!are_equal(multiplied, vectors[i], precision))
                {
                        error("Failed to solve:\nvectors = " + to_string(vectors[i])
                              + "\nmultiplied = " + to_string(multiplied));
                }
        }
}

template <std::size_t N, std::size_t M, typename T>
void test_solve_matrix(const int count, const std::type_identity_t<T>& precision)
{
        write("Test solve (" + to_string(N) + ", " + to_string(M) + "), " + type_name<T>());

        const std::vector<std::array<Vector<N, T>, N>> matrices = random_matrices<N, N, T>(count);
        const std::vector<std::array<Vector<M, T>, N>> columns = random_matrices<N, M, T>(count);

        const std::vector<std::array<Vector<M, T>, N>> solved = [&]
        {
                std::vector<std::array<Vector<M, T>, N>> res(count);
                const TimePoint start_time = time();
                for (int i = 0; i < count; ++i)
                {
                        res[i] = solve_gauss(matrices[i], columns[i]);
                }
                write("Time = " + to_string_fixed(duration_from(start_time), 5) + " s");
                return res;
        }();

        for (int i = 0; i < count; ++i)
        {
                std::array<Vector<M, T>, N> multiplied;
                for (std::size_t r = 0; r < N; ++r)
                {
                        for (std::size_t c = 0; c < M; ++c)
                        {
                                T sum = 0;
                                for (std::size_t s = 0; s < N; ++s)
                                {
                                        sum += matrices[i][r][s] * solved[i][s][c];
                                }
                                multiplied[r][c] = sum;
                        }
                }
                if (!are_equal(multiplied, columns[i], precision))
                {
                        error("Failed to solve:\ncolumns = " + to_string(columns[i])
                              + "\nmultiplied = " + to_string(multiplied));
                }
        }
}

template <std::size_t N, typename T>
void test_solve(const int count, const std::type_identity_t<T>& precision)
{
        test_solve_vector<N, T>(count, precision);
        test_solve_matrix<N, 1, T>(count, precision);
        test_solve_matrix<N, 2, T>(count, precision);
        test_solve_matrix<N, 3, T>(count, precision);
}

template <typename T>
void test_solve(const int count, const std::type_identity_t<T>& precision)
{
        test_solve<1, T>(count, precision);
        test_solve<2, T>(count, precision);
        test_solve<3, T>(count, precision);
        test_solve<4, T>(count, precision);
        test_solve<5, T>(count, precision);
        test_solve<6, T>(count, precision);
        test_solve<7, T>(count, precision);
        test_solve<8, T>(count, precision);
}

void test()
{
        LOG("Test linear solve");
        test_solve<double>(1000, 1e-6);
        test_solve<long double>(100, 1e-9);
        LOG("Test linear solve passed");
}

TEST_SMALL("Linear Solve", test)
}
}
