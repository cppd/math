/*
Copyright (C) 2017-2023 Topological Manifold

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

#include "../cholesky.h"

#include <src/com/error.h>
#include <src/com/log.h>
#include <src/com/random/pcg.h>
#include <src/test/test.h>

#include <random>
#include <utility>

namespace ns::numerical
{
namespace
{
template <typename T>
[[nodiscard]] bool equal(const T a, const T b, const T precision)
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
[[nodiscard]] bool equal(const Matrix<N, N, T>& a, const Matrix<N, N, T>& b, const T precision)
{
        for (std::size_t r = 0; r < N; ++r)
        {
                for (std::size_t c = 0; c < N; ++c)
                {
                        if (!equal(a(r, c), b(r, c), precision))
                        {
                                return false;
                        }
                }
        }
        return true;
}

template <std::size_t N, typename T>
[[nodiscard]] bool lower_triangular(const Matrix<N, N, T>& a)
{
        for (std::size_t r = 0; r < N; ++r)
        {
                for (std::size_t c = r + 1; c < N; ++c)
                {
                        if (!(a(r, c) == 0))
                        {
                                return false;
                        }
                }
        }
        return true;
}

template <std::size_t N, typename T>
void test(const Matrix<N, N, T>& matrix, const std::type_identity_t<T> precision)
{
        const Matrix<N, N, T> decomposition = cholesky_decomposition_lower_triangular(matrix);

        if (!lower_triangular(decomposition))
        {
                error("Decomposition is not lower triangular\n" + to_string(decomposition));
        }

        const Matrix<N, N, T> check = decomposition * decomposition.transposed();

        if (!equal(check, matrix, precision))
        {
                error("Failed to decompose\nmatrix\n" + to_string(check) + "\nis not equal to the original matrix\n"
                      + to_string(matrix));
        }
}

template <std::size_t N, typename T>
[[nodiscard]] Matrix<N, N, T> random_matrix(PCG& pcg)
{
        std::uniform_real_distribution<T> urd(-10, 10);
        Matrix<N, N, T> res;
        for (std::size_t r = 0; r < N; ++r)
        {
                for (std::size_t c = 0; c < N; ++c)
                {
                        res(r, c) = urd(pcg);
                }
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] Matrix<N, N, T> positive_diagonal_matrix(PCG& pcg)
{
        std::uniform_real_distribution<T> urd(1, 10);
        Matrix<N, N, T> res(0);
        for (std::size_t i = 0; i < N; ++i)
        {
                res(i, i) = urd(pcg);
        }
        return res;
}

template <std::size_t N, typename T>
[[nodiscard]] Matrix<N, N, T> positive_definite_matrix(PCG& pcg)
{
        const Matrix<N, N, T> r = random_matrix<N, T>(pcg);
        const Matrix<N, N, T> d = positive_diagonal_matrix<N, T>(pcg);
        return r * d * r.transposed();
}

template <typename T>
void test(const std::type_identity_t<T> precision, PCG& pcg)
{
        [&]<unsigned... I>(std::integer_sequence<unsigned, I...>&&)
        {
                (test(positive_definite_matrix<1 + I, T>(pcg), precision), ...);
        }(std::make_integer_sequence<unsigned, 10>());
}

void test_cholesky()
{
        LOG("Test the Cholesky decomposition");
        PCG pcg;
        test<double>(1e-12, pcg);
        test<long double>(1e-15, pcg);
        LOG("Test the Cholesky decomposition passed");
}

TEST_SMALL("Cholesky Decomposition", test_cholesky)
}
}
