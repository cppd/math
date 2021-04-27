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

/*
Jakob Andreas Bærentzen, Jens Gravesen, François Anton, Henrik Aanæs.
Guide to Computational Geometry Processing. Foundations, Algorithms, and Methods.
Springer-Verlag London, 2012.

17.1.1 Computing Point Normals
*/

#pragma once

#include "eigen.h"
#include "matrix.h"
#include "orthogonal.h"
#include "vec.h"

#include <src/com/error.h>
#include <src/com/print.h>
#include <src/com/type/limit.h>

#include <vector>

namespace ns::numerical
{
template <std::size_t N, typename T>
Vector<N, T> point_normal(const std::vector<Vector<N, T>>& points)
{
        if (points.size() < N)
        {
                error("At least " + to_string(N) + " points are required for point normal");
        }

        Vector<N, T> p0(0);
        for (const Vector<N, T>& p : points)
        {
                p0 += p;
        }
        p0 /= points.size();

        Matrix<N, N, T> covariance_matrix;
        for (std::size_t i = 0; i < N; ++i)
        {
                covariance_matrix.row(i) = Vector<N, T>(0);
        }
        for (Vector<N, T> p : points)
        {
                p -= p0;
                for (std::size_t i = 0; i < N; ++i)
                {
                        for (std::size_t j = i; j < N; ++j)
                        {
                                covariance_matrix(i, j) += p[i] * p[j];
                        }
                }
        }

        T max = limits<T>::lowest();
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                for (std::size_t j = i + 1; j < N; ++j)
                {
                        max = std::max(max, std::abs(covariance_matrix(i, j)));
                }
        }

        const T tolerance = max * (100 * limits<T>::epsilon());
        Vector<N, T> eigenvalues;
        std::array<Vector<N, T>, N> eigenvectors;
        eigen_symmetric_upper_triangular(covariance_matrix, tolerance, &eigenvalues, &eigenvectors);

        std::size_t min_i = 0;
        T min = eigenvalues[0];
        for (std::size_t i = 1; i < N; ++i)
        {
                if (min > eigenvalues[i])
                {
                        min_i = i;
                        min = eigenvalues[i];
                }
        }
        std::array<Vector<N, T>, N - 1> vectors;
        std::size_t n = 0;
        for (std::size_t i = 0; i < min_i; ++i)
        {
                vectors[n++] = eigenvectors[i];
        }
        for (std::size_t i = min_i + 1; i < N; ++i)
        {
                vectors[n++] = eigenvectors[i];
        }

        return ortho_nn(vectors).normalized();
}
}
