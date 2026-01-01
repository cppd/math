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

#pragma once

#include "matrix.h"
#include "vector.h"

#include <src/com/error.h>

#include <cstddef>

namespace ns::numerical::transform
{
// template <typename T>
// [[nodiscard]] Matrix<4, 4, T> look_at(const Vector<3, T>& eye, const Vector<3, T>& center, const Vector<3, T>& up)
// {
//         const Vector<3, T> f = (center - eye).normalized();
//         const Vector<3, T> s = cross(f, up).normalized();
//         const Vector<3, T> u = cross(s, f).normalized();
//
//         const T se = dot(s, eye);
//         const T ue = dot(u, eye);
//         const T fe = dot(f, eye);
//
//         return {
//                 { s[0],  s[1],  s[2], -se},
//                 { u[0],  u[1],  u[2], -ue},
//                 {-f[0], -f[1], -f[2],  fe},
//                 {    0,     0,     0,   1},
//         };
// }

template <std::size_t N, typename T>
[[nodiscard]] constexpr Matrix<N + 1, N + 1, T> scale(const Vector<N, T>& v)
{
        Matrix<N + 1, N + 1, T> res = IDENTITY_MATRIX<N + 1, T>;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i, i] = v[i];
        }
        return res;
}

template <typename T, typename... V>
[[nodiscard]] constexpr Matrix<sizeof...(V) + 1, sizeof...(V) + 1, T> scale(const V&... v)
{
        return scale(Vector<sizeof...(V), T>(v...));
}

template <std::size_t N, typename T>
[[nodiscard]] constexpr Matrix<N + 1, N + 1, T> translate(const Vector<N, T>& v)
{
        Matrix<N + 1, N + 1, T> res = IDENTITY_MATRIX<N + 1, T>;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i, N] = v[i];
        }
        return res;
}

template <typename T, typename... V>
[[nodiscard]] constexpr Matrix<sizeof...(V) + 1, sizeof...(V) + 1, T> translate(const V&... v)
{
        return translate(Vector<sizeof...(V), T>(v...));
}

template <std::size_t N, typename T>
class MatrixVectorMultiplier final
{
        Matrix<N, N, T> matrix_;

public:
        explicit MatrixVectorMultiplier(const Matrix<N, N, T>& matrix)
                : matrix_(matrix)
        {
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        if (matrix_[N - 1, i] != 0)
                        {
                                error("Wrong matrix for matrix-vector multiplier");
                        }
                }

                if (matrix_[N - 1, N - 1] != 1)
                {
                        error("Wrong matrix for matrix-vector multiplier");
                }
        }

        [[nodiscard]] Vector<N - 1, T> operator()(const Vector<N - 1, T>& v) const
        {
                Vector<N - 1, T> res;
                for (std::size_t r = 0; r < N - 1; ++r)
                {
                        res[r] = matrix_[r, 0] * v[0];
                        for (std::size_t c = 1; c < N - 1; ++c)
                        {
                                res[r] += matrix_[r, c] * v[c];
                        }
                        res[r] += matrix_[r, N - 1];
                }
                return res;
        }
};
}
