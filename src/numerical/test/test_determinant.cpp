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

#include "../determinant.h"
#include "../gauss.h"

#include <src/com/arrays.h>
#include <src/com/chrono.h>
#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/print.h>
#include <src/com/random/engine.h>
#include <src/com/type/name.h>
#include <src/test/test.h>

#include <random>

namespace ns::numerical::determinant_implementation
{
namespace
{
// clang-format off
template<typename T>
constexpr std::array VECTORS = std::to_array<Vector<7, T>>
({
{10,  2,   3,   4,   5,   6,   7},
{ 8, 90,  10,  11,  12,  13,  14},
{15, 16, 170,  18,  19,  20,  21},
{22, 23,  24, 250,  26,  27,  28},
{29, 30,  31,  32, 330,  34,  35},
{36, 37,  38,  39,  40, 410,  42},
{43, 44,  45,  46,  47,  48, 490}
});
// clang-format on

template <typename T>
constexpr bool TEST_COFACTOR_EXPANSION =
        (1'868'201'030'776'500
         == determinant_cofactor_expansion(VECTORS<T>, SEQUENCE_UCHAR_ARRAY<7>, SEQUENCE_UCHAR_ARRAY<7>))
        && (-28
            == determinant_cofactor_expansion(
                    VECTORS<T>,
                    std::to_array<unsigned char>({2, 4}),
                    std::to_array<unsigned char>({3, 5})));
static_assert(TEST_COFACTOR_EXPANSION<long long>);
static_assert(TEST_COFACTOR_EXPANSION<__int128>);
static_assert(TEST_COFACTOR_EXPANSION<double>);
static_assert(TEST_COFACTOR_EXPANSION<long double>);
static_assert(TEST_COFACTOR_EXPANSION<__float128>);

template <typename T>
constexpr bool TEST_ROW_REDUCTION = 1'868'201'030'776'499 <= determinant_gauss(VECTORS<T>)
                                    && determinant_gauss(VECTORS<T>) <= 1'868'201'030'776'500;
static_assert(TEST_ROW_REDUCTION<float>);
static_assert(TEST_ROW_REDUCTION<double>);
static_assert(TEST_ROW_REDUCTION<long double>);
static_assert(TEST_ROW_REDUCTION<__float128>);

template <typename T>
constexpr bool test_cofactor_expansion()
{
        Vector<7, T> d;
        for (int i = 0; i < 7; ++i)
        {
                d[i] = determinant_cofactor_expansion(
                        del_elem(VECTORS<T>, 6), SEQUENCE_UCHAR_ARRAY<6>, del_elem(SEQUENCE_UCHAR_ARRAY<7>, i));
        }
        return d
               == Vector<7, T>(
                       -2555210922012, 336840375312, -206347990212, 159370731576, -135170325612, 120413980512,
                       4125807482688);
}
static_assert(test_cofactor_expansion<long long>());
static_assert(test_cofactor_expansion<__int128>());
static_assert(test_cofactor_expansion<double>());
static_assert(test_cofactor_expansion<long double>());
static_assert(test_cofactor_expansion<__float128>());

template <typename T>
constexpr bool test_row_reduction()
{
        const auto cmp = [](const T& v, const long long& c)
        {
                return v > c - T(0.1) && v < c + T(0.1);
        };
        constexpr Vector<7, long long> VECTOR(
                -2555210922012, 336840375312, -206347990212, 159370731576, -135170325612, 120413980512, 4125807482688);
        for (int i = 0; i < 7; ++i)
        {
                if (!cmp(determinant_gauss(del_elem(VECTORS<T>, 6), i), VECTOR[i]))
                {
                        return false;
                }
        }
        return true;
}
static_assert(test_row_reduction<double>());
static_assert(test_row_reduction<long double>());
static_assert(test_row_reduction<__float128>());

//

template <typename T>
bool are_equal(const T& a, const T& b, const T& precision)
{
        if (a == b)
        {
                return true;
        }
        const T rel = std::abs(a - b) / std::max(std::abs(a), std::abs(b));
        return (rel < precision);
}

template <std::size_t ROWS, std::size_t COLUMNS, typename T>
std::vector<std::array<Vector<COLUMNS, T>, ROWS>> random_matrices(const int count)
{
        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();
        std::uniform_real_distribution<T> urd(-10, 10);
        std::vector<std::array<Vector<COLUMNS, T>, ROWS>> res(count);
        for (int i = 0; i < count; ++i)
        {
                for (std::size_t r = 0; r < ROWS; ++r)
                {
                        for (std::size_t c = 0; c < COLUMNS; ++c)
                        {
                                res[i][r][c] = urd(random_engine);
                        }
                }
        }
        return res;
}

template <std::size_t N, typename T>
void test_determinant(const int count, const std::type_identity_t<T>& precision)
{
        LOG("Test determinant, " + to_string(N) + ", " + type_name<T>());

        const std::vector<std::array<Vector<N, T>, N>> matrices = random_matrices<N, N, T>(count);

        const std::vector<T> cofactor_expansion = [&]
        {
                std::vector<T> res(count);
                const Clock::time_point start_time = Clock::now();
                for (int i = 0; i < count; ++i)
                {
                        res[i] = determinant_cofactor_expansion(
                                matrices[i], SEQUENCE_UCHAR_ARRAY<N>, SEQUENCE_UCHAR_ARRAY<N>);
                }
                LOG("Time = " + to_string_fixed(duration_from(start_time), 5) + " s, cofactor expansion");
                return res;
        }();

        const std::vector<T> row_reduction = [&]
        {
                std::vector<T> res(count);
                const Clock::time_point start_time = Clock::now();
                for (int i = 0; i < count; ++i)
                {
                        res[i] = determinant_gauss(matrices[i]);
                }
                LOG("Time = " + to_string_fixed(duration_from(start_time), 5) + " s, row reduction");
                return res;
        }();

        const std::vector<T> determinants = [&]
        {
                std::vector<T> res(count);
                const Clock::time_point start_time = Clock::now();
                for (int i = 0; i < count; ++i)
                {
                        res[i] = determinant(matrices[i]);
                }
                LOG("Time = " + to_string_fixed(duration_from(start_time), 5) + " s, determinant");
                return res;
        }();

        for (int i = 0; i < count; ++i)
        {
                if (!are_equal(cofactor_expansion[i], row_reduction[i], precision))
                {
                        error("Determinants are not equal:\ncofactor_expansion = " + to_string(cofactor_expansion[i])
                              + "\nrow_reduction = " + to_string(row_reduction[i]) + "\n" + to_string(matrices[i]));
                }
                if (!(determinants[i] == cofactor_expansion[i] || determinants[i] == row_reduction[i]))
                {
                        error("Determinant error:\ndeterminant = " + to_string(determinants[i])
                              + "\ncofactor_expansion = " + to_string(cofactor_expansion[i])
                              + "\nrow_reduction = " + to_string(row_reduction[i]));
                }
        }
}

template <std::size_t N, typename T>
void test_determinant_column(const int count, const std::type_identity_t<T>& precision)
{
        LOG("Test determinant column, " + to_string(N) + ", " + type_name<T>());

        const std::vector<std::array<Vector<N, T>, N - 1>> matrices = random_matrices<N - 1, N, T>(count);

        const std::vector<Vector<N, T>> cofactor_expansion = [&]
        {
                std::vector<Vector<N, T>> res(count);
                const Clock::time_point start_time = Clock::now();
                for (int i = 0; i < count; ++i)
                {
                        for (std::size_t c = 0; c < N; ++c)
                        {
                                res[i][c] = determinant_cofactor_expansion(
                                        matrices[i], SEQUENCE_UCHAR_ARRAY<N - 1>, del_elem(SEQUENCE_UCHAR_ARRAY<N>, c));
                        }
                }
                LOG("Time = " + to_string_fixed(duration_from(start_time), 5) + " s, cofactor expansion");
                return res;
        }();

        const std::vector<Vector<N, T>> row_reduction = [&]
        {
                std::vector<Vector<N, T>> res(count);
                const Clock::time_point start_time = Clock::now();
                for (int i = 0; i < count; ++i)
                {
                        for (std::size_t c = 0; c < N; ++c)
                        {
                                res[i][c] = determinant_gauss(matrices[i], c);
                        }
                }
                LOG("Time = " + to_string_fixed(duration_from(start_time), 5) + " s, row reduction");
                return res;
        }();

        const std::vector<Vector<N, T>> determinants = [&]
        {
                std::vector<Vector<N, T>> res(count);
                const Clock::time_point start_time = Clock::now();
                for (int i = 0; i < count; ++i)
                {
                        for (std::size_t c = 0; c < N; ++c)
                        {
                                res[i][c] = determinant(matrices[i], c);
                        }
                }
                LOG("Time = " + to_string_fixed(duration_from(start_time), 5) + " s, determinant");
                return res;
        }();

        for (int i = 0; i < count; ++i)
        {
                for (std::size_t c = 0; c < N; ++c)
                {
                        if (!are_equal(cofactor_expansion[i][c], row_reduction[i][c], precision))
                        {
                                error("Determinants are not equal:\ncofactor_expansion = "
                                      + to_string(cofactor_expansion[i]) + "\nrow_reduction = "
                                      + to_string(row_reduction[i]) + "\n" + to_string(matrices[i]));
                        }
                        if (!(determinants[i][c] == cofactor_expansion[i][c]
                              || determinants[i][c] == row_reduction[i][c]))
                        {
                                error("Determinant error:\ndeterminant = " + to_string(determinants[i])
                                      + "\ncofactor_expansion = " + to_string(cofactor_expansion[i])
                                      + "\nrow_reduction = " + to_string(row_reduction[i]));
                        }
                }
        }
}

template <typename T>
void test_determinant(const int count, const std::type_identity_t<T>& precision)
{
        test_determinant<1, T>(count, precision);
        test_determinant<2, T>(count, precision);
        test_determinant<3, T>(count, precision);
        test_determinant<4, T>(count, precision);
        test_determinant<5, T>(count, precision);
        test_determinant<6, T>(count, precision);
        test_determinant<7, T>(count, precision);
        test_determinant<8, T>(count, precision);
}

template <typename T>
void test_determinant_column(const int count, const std::type_identity_t<T>& precision)
{
        test_determinant_column<2, T>(count, precision);
        test_determinant_column<3, T>(count, precision);
        test_determinant_column<4, T>(count, precision);
        test_determinant_column<5, T>(count, precision);
        test_determinant_column<6, T>(count, precision);
        test_determinant_column<7, T>(count, precision);
        test_determinant_column<8, T>(count, precision);
        test_determinant_column<9, T>(count, precision);
}

void test()
{
        LOG("Test determinant");
        test_determinant<double>(500, 1e-8);
        test_determinant<long double>(50, 1e-11);
        test_determinant_column<double>(500, 1e-8);
        test_determinant_column<long double>(50, 1e-11);
        LOG("Test determinant passed");
}

TEST_SMALL("Determinant", test)
}
}
