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

#pragma once

#include "matrix.h"

namespace ns::numerical
{
template <std::size_t N, std::size_t M, typename T>
struct Filter final
{
        // state mean
        Vector<N, T> x;
        // state covariance
        Matrix<N, N, T> p;
        // state transition function
        Matrix<N, N, T> f;
        // process covariance
        Matrix<N, N, T> q;
        // measurement function
        Matrix<M, N, T> h;
        // measurement covariance
        Matrix<M, M, T> r;
};
}
