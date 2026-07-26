/*
Copyright (C) 2017-2026 Topological Manifold

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

/*
Steven C. Chapra, Raymond P. Canale.
Numerical Methods for Engineers. Seventh edition.
McGraw-Hill Education, 2015.

11.1.2 Cholesky Decomposition
*/

#pragma once

#include "matrix.h"

#include <src/com/print.h>

#include <cmath>
#include <cstddef>
#include <exception>
#include <string>
#include <utility>

namespace ns::numerical
{
class CholeskyException final : public std::exception
{
        std::string msg_;

public:
        explicit CholeskyException(std::string&& msg)
                : msg_(std::move(msg))
        {
        }

        [[nodiscard]] const char* what() const noexcept override
        {
                return msg_.c_str();
        }
};

namespace cholesky_implementation
{
template <std::size_t N, typename T>
[[nodiscard]] T sqrt(const T& v, const Matrix<N, N, T>& m, const Matrix<N, N, T>& res)
{
        if (v >= 0)
        {
                return std::sqrt(v);
        }
        throw CholeskyException(
                "The Cholesky decomposition: matrix is not positive definite, sqrt(" + to_string(v) + ")\n"
                + to_string(m) + "\n" + to_string(res));
}

template <std::size_t N, typename T>
void set_non_diagonal(const Matrix<N, N, T>& m, const std::size_t r, const std::size_t c, Matrix<N, N, T>& res)
{
        T sum = 0;
        for (std::size_t i = 0; i < c; ++i)
        {
                sum += res[c, i] * res[r, i];
        }
        res[r, c] = (m[r, c] - sum) / res[c, c];
}

template <std::size_t N, typename T>
void set_diagonal(const Matrix<N, N, T>& m, const std::size_t r, Matrix<N, N, T>& res)
{
        T sum = 0;
        for (std::size_t i = 0; i < r; ++i)
        {
                const T v = res[r, i];
                sum += v * v;
        }
        res[r, r] = sqrt(m[r, r] - sum, m, res);
}
}

template <std::size_t N, typename T>
[[nodiscard]] Matrix<N, N, T> cholesky_decomposition_lower_triangular(const Matrix<N, N, T>& m)
{
        static_assert(N > 0);

        namespace impl = cholesky_implementation;

        Matrix<N, N, T> res{ZERO_MATRIX};
        for (std::size_t r = 0; r < N; ++r)
        {
                for (std::size_t c = 0; c < r; ++c)
                {
                        impl::set_non_diagonal(m, r, c, res);
                }
                impl::set_diagonal(m, r, res);
        }
        return res;
}
}
