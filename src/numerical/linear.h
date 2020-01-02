/*
Copyright (C) 2017-2020 Topological Manifold

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

#include "gauss.h"

namespace numerical
{
template <size_t Size, typename T>
Vector<Size, T> solve(Matrix<Size, Size, T>&& a, const Vector<Size, T>& b)
{
        Vector<Size, T> x(b);

        solve_gauss<Size, T>(&a, &x);

        return x;
}

template <size_t Size, typename T>
Vector<Size, T> solve(const Matrix<Size, Size, T>& a, const Vector<Size, T>& b)
{
        return solve(Matrix<Size, Size, T>(a), b);
}

template <size_t Size, typename T>
Matrix<Size, Size, T> inverse(Matrix<Size, Size, T>&& m)
{
        Matrix<Size, Size, T> b(1);

        solve_gauss<Size, Size, T>(&m, &b);

        return b;
}

template <size_t Size, typename T>
Matrix<Size, Size, T> inverse(const Matrix<Size, Size, T>& m)
{
        return inverse(Matrix<Size, Size, T>(m));
}

template <size_t Size, typename T>
T determinant(Matrix<Size, Size, T>&& m)
{
        return determinant_gauss<Size, T>(&m);
}

template <size_t Size, typename T>
T determinant(const Matrix<Size, Size, T>& m)
{
        return determinant(Matrix<Size, Size, T>(m));
}
}
