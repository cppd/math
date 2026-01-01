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

namespace ns::numerical
{
class CholeskyException final : public std::exception
{
        std::string msg_;

public:
        explicit CholeskyException(const std::string& msg)
                : msg_("The Cholesky decomposition: matrix is not positive definite, " + msg)
        {
        }

        [[nodiscard]] const char* what() const noexcept override
        {
                return msg_.c_str();
        }
};

template <std::size_t N, typename T>
[[nodiscard]] Matrix<N, N, T> cholesky_decomposition_lower_triangular(const Matrix<N, N, T>& a)
{
        Matrix<N, N, T> l{ZERO_MATRIX};

        for (std::size_t k = 0; k < N; ++k)
        {
                for (std::size_t i = 0; i < k; ++i)
                {
                        T sum = 0;
                        for (std::size_t j = 0; j < i; ++j)
                        {
                                sum += l[i, j] * l[k, j];
                        }
                        l[k, i] = (a[k, i] - sum) / l[i, i];
                }

                T sum = 0;
                for (std::size_t j = 0; j < k; ++j)
                {
                        const T v = l[k, j];
                        sum += v * v;
                }

                const T v = a[k, k] - sum;

                if (!(v >= 0))
                {
                        throw CholeskyException("sqrt(" + to_string(v) + ")\n" + to_string(a) + "\n" + to_string(l));
                }

                l[k, k] = std::sqrt(v);
        }

        return l;
}
}
