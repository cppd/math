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
Jakob Andreas Bærentzen, Jens Gravesen, François Anton, Henrik Aanæs.
Guide to Computational Geometry Processing. Foundations, Algorithms, and Methods.
Springer-Verlag London, 2012.

17.1.1 Computing Point Normals
*/

#pragma once

#include "complement.h"
#include "covariance.h"
#include "eigen.h"
#include "matrix.h"
#include "vector.h"

#include <src/com/arrays.h>
#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>

#include <cstddef>
#include <vector>

namespace ns::numerical
{
namespace normal_implementation
{
template <std::size_t N, typename T>
Eigen<N, T> covariance_matrix_eigen_for_points(const std::vector<Vector<N, T>>& points)
{
        const Matrix<N, N, T> covariance_matrix = covariance_matrix_simple(points);

        const T tolerance = [&]
        {
                T max = Limits<T>::lowest();
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        for (std::size_t j = i + 1; j < N; ++j)
                        {
                                max = std::max(max, std::abs(covariance_matrix[i, j]));
                        }
                }
                return max * (100 * Limits<T>::epsilon());
        }();

        return eigen_symmetric_upper_triangular(covariance_matrix, tolerance);
}

template <std::size_t N, typename T>
std::size_t min_value_index(const Vector<N, T>& v)
{
        static_assert(N > 0);

        std::size_t index = 0;
        T min = v[0];
        for (std::size_t i = 1; i < N; ++i)
        {
                if (v[i] < min)
                {
                        index = i;
                        min = v[i];
                }
        }
        return index;
}
}

template <std::size_t N, typename T>
Vector<N, T> point_normal(const std::vector<Vector<N, T>>& points)
{
        namespace impl = normal_implementation;

        if (points.size() < N)
        {
                error("At least " + to_string(N) + " points are required for point normal");
        }

        const Eigen eigen = impl::covariance_matrix_eigen_for_points(points);

        const auto min_value_index = impl::min_value_index(eigen.values);

        const auto vectors = del_elem(eigen.vectors, min_value_index);

        return orthogonal_complement(vectors).normalized();
}
}
