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

/*
Steven J. Leon.
Linear Algebra with Applications. Ninth Edition.
Pearson Education, 2015.

2.3 Additional Topics and Applications
The Adjoint of a Matrix
Cramer’s Rule
*/

#pragma once

#include "gauss.h"
#include "identity.h"
#include "vector.h"

#include <src/com/type/concept.h>

#include <array>
#include <cstddef>

namespace ns::numerical
{
namespace solve_implementation
{
template <std::size_t N, typename T>
constexpr Vector<N, T> solve_cofactor_expansion(const std::array<Vector<N, T>, N>& a, const Vector<N, T>& b)
{
        // Cramer’s rule

        if constexpr (N == 1)
        {
                return Vector<1, T>(b[0] / a[0][0]);
        }
        else if constexpr (N == 2)
        {
                const T d = a[0][0] * a[1][1] - a[0][1] * a[1][0];
                Vector<2, T> res;
                res[0] = (b[0] * a[1][1] - b[1] * a[0][1]) / d;
                res[1] = (b[1] * a[0][0] - b[0] * a[1][0]) / d;
                return res;
        }
        else if constexpr (N == 3)
        {
                const T c00 = a[1][1] * a[2][2] - a[1][2] * a[2][1];
                const T c10 = a[0][2] * a[2][1] - a[0][1] * a[2][2];
                const T c20 = a[0][1] * a[1][2] - a[0][2] * a[1][1];

                const T d = a[0][0] * c00 + a[1][0] * c10 + a[2][0] * c20;

                Vector<3, T> res;

                res[0] = (b[0] * c00 + b[1] * c10 + b[2] * c20) / d;

                const T c01 = a[1][2] * a[2][0] - a[1][0] * a[2][2];
                const T c11 = a[0][0] * a[2][2] - a[0][2] * a[2][0];
                const T c21 = a[0][2] * a[1][0] - a[0][0] * a[1][2];
                res[1] = (b[0] * c01 + b[1] * c11 + b[2] * c21) / d;

                const T c02 = a[1][0] * a[2][1] - a[1][1] * a[2][0];
                const T c12 = a[0][1] * a[2][0] - a[0][0] * a[2][1];
                const T c22 = a[0][0] * a[1][1] - a[0][1] * a[1][0];
                res[2] = (b[0] * c02 + b[1] * c12 + b[2] * c22) / d;

                return res;
        }
        else
        {
                static_assert(false);
        }
}

template <std::size_t N, typename T>
constexpr std::array<Vector<N, T>, N> inverse_cofactor_expansion(const std::array<Vector<N, T>, N>& a)
{
        // Adjoint

        if constexpr (N == 1)
        {
                return {Vector<1, T>(T{1} / a[0][0])};
        }
        else if constexpr (N == 2)
        {
                const T d = a[0][0] * a[1][1] - a[0][1] * a[1][0];
                std::array<Vector<2, T>, 2> res;
                res[0][0] = a[1][1] / d;
                res[0][1] = -a[0][1] / d;
                res[1][0] = -a[1][0] / d;
                res[1][1] = a[0][0] / d;
                return res;
        }
        else if constexpr (N == 3)
        {
                const T c00 = a[1][1] * a[2][2] - a[1][2] * a[2][1];
                const T c10 = a[0][2] * a[2][1] - a[0][1] * a[2][2];
                const T c20 = a[0][1] * a[1][2] - a[0][2] * a[1][1];

                const T d = a[0][0] * c00 + a[1][0] * c10 + a[2][0] * c20;

                std::array<Vector<3, T>, 3> res;

                res[0][0] = c00 / d;
                res[0][1] = c10 / d;
                res[0][2] = c20 / d;

                res[1][0] = (a[1][2] * a[2][0] - a[1][0] * a[2][2]) / d;
                res[1][1] = (a[0][0] * a[2][2] - a[0][2] * a[2][0]) / d;
                res[1][2] = (a[0][2] * a[1][0] - a[0][0] * a[1][2]) / d;

                res[2][0] = (a[1][0] * a[2][1] - a[1][1] * a[2][0]) / d;
                res[2][1] = (a[0][1] * a[2][0] - a[0][0] * a[2][1]) / d;
                res[2][2] = (a[0][0] * a[1][1] - a[0][1] * a[1][0]) / d;

                return res;
        }
        else
        {
                static_assert(false);
        }
}
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Vector<N, T> linear_solve(const std::array<Vector<N, T>, N>& a, const Vector<N, T>& b)
{
        static_assert(FloatingPoint<T>);
        static_assert(N > 0);

        if constexpr (N <= 3)
        {
                return solve_implementation::solve_cofactor_expansion(a, b);
        }
        else
        {
                return solve_gauss(a, b);
        }
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr std::array<Vector<N, T>, N> inverse(const std::array<Vector<N, T>, N>& a)
{
        static_assert(FloatingPoint<T>);
        static_assert(N > 0);

        if constexpr (N <= 3)
        {
                return solve_implementation::inverse_cofactor_expansion(a);
        }
        else
        {
                return solve_gauss(a, IDENTITY_ARRAY<N, T>);
        }
}
}
