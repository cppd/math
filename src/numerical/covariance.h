/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <cstddef>
#include <vector>

namespace ns::numerical
{
namespace covariance_implementation
{
template <std::size_t N, typename T>
Vector<N, T> average(const std::vector<Vector<N, T>>& data)
{
        Vector<N, T> res(0);
        for (const Vector<N, T>& p : data)
        {
                res += p;
        }
        return res / static_cast<T>(data.size());
}
}

template <std::size_t N, typename T>
Matrix<N, N, T> covariance_matrix_simple(const std::vector<Vector<N, T>>& data)
{
        namespace impl = covariance_implementation;

        const Vector<N, T> average = impl::average(data);

        Matrix<N, N, T> res;

        for (std::size_t i = 0; i < N; ++i)
        {
                res.row(i) = Vector<N, T>(0);
        }

        for (const Vector<N, T>& d : data)
        {
                const Vector<N, T> v = d - average;
                for (std::size_t i = 0; i < N; ++i)
                {
                        for (std::size_t j = i; j < N; ++j)
                        {
                                res[i, j] += v[i] * v[j];
                        }
                }
        }

        return res;
}

template <std::size_t N, typename T>
Matrix<N, N, T> covariance_matrix_full(const std::vector<Vector<N, T>>& data)
{
        Matrix<N, N, T> res = covariance_matrix_simple(data);

        const T size = data.size();

        for (std::size_t i = 0; i < N; ++i)
        {
                for (std::size_t j = i; j < N; ++j)
                {
                        res[i, j] /= size;
                }
        }

        for (std::size_t i = 0; i < N; ++i)
        {
                for (std::size_t j = 0; j < i; ++j)
                {
                        res[i, j] = res[j, i];
                }
        }

        return res;
}
}
