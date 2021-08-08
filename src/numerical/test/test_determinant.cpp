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
#include "../matrix.h"

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

constexpr long long DETERMINANT = 1'868'201'030'776'500;

static_assert(DETERMINANT == determinant(VECTORS<long long>));
static_assert(DETERMINANT == determinant(VECTORS<__int128>));
static_assert(DETERMINANT == determinant(VECTORS<double>));
static_assert(DETERMINANT == determinant(VECTORS<long double>));
static_assert(DETERMINANT == determinant(VECTORS<__float128>));

static_assert(
        -28 == determinant(VECTORS<int>, std::to_array<unsigned char>({2, 4}), std::to_array<unsigned char>({3, 5})));

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

template <std::size_t N, typename T>
std::vector<std::array<Vector<N, T>, N>> random_matrices(const int count)
{
        std::mt19937_64 random_engine = create_engine<std::mt19937_64>();
        std::uniform_real_distribution<T> urd(-10, 10);
        std::vector<std::array<Vector<N, T>, N>> res(count);
        for (int i = 0; i < count; ++i)
        {
                for (std::size_t j = 0; j < N; ++j)
                {
                        for (std::size_t k = 0; k < N; ++k)
                        {
                                res[i][j][k] = urd(random_engine);
                        }
                }
        }
        return res;
}

template <std::size_t N, typename T>
void test_determinant(const int count, const std::type_identity_t<T>& precision)
{
        LOG("Test determinant, " + to_string(N) + ", " + type_name<T>());
        const std::vector<std::array<Vector<N, T>, N>> matrices = random_matrices<N, T>(count);

        std::vector<T> cofactor_expansion(count);
        std::vector<T> row_reduction(count);

        {
                TimePoint start_time = time();
                for (int i = 0; i < count; ++i)
                {
                        cofactor_expansion[i] = determinant(matrices[i]);
                }
                LOG("Time = " + to_string_fixed(duration_from(start_time), 5) + " s, cofactor expansion");
        }

        {
                TimePoint start_time = time();
                for (int i = 0; i < count; ++i)
                {
                        Matrix<N, N, T> matrix(matrices[i]);
                        row_reduction[i] = determinant_gauss(&matrix);
                }
                LOG("Time = " + to_string_fixed(duration_from(start_time), 5) + " s, row reduction");
        }

        for (int i = 0; i < count; ++i)
        {
                if (!are_equal(cofactor_expansion[i], row_reduction[i], precision))
                {
                        error("Determinants are not equal:\ncofactor_expansion = " + to_string(cofactor_expansion[i])
                              + "\nrow_reduction = " + to_string(row_reduction[i]) + "\n" + to_string(matrices[i]));
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

void test()
{
        LOG("Test determinant");
        test_determinant<double>(1'000, 1e-8);
        test_determinant<long double>(100, 1e-11);
        LOG("Test determinant passed");
}

TEST_SMALL("Determinant", test)
}
}
