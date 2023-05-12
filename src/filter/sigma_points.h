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

/*
Roger R Labbe Jr.
Kalman and Bayesian Filters in Python.

10.6 Van der Merwe’s Scaled Sigma Point Algorithm
10.11 Implementation of the UKF
*/

#pragma once

#include <src/com/exponent.h>
#include <src/numerical/cholesky.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <array>

namespace ns::filter
{
template <std::size_t N, typename T>
class SigmaPoints final
{
        static_assert(std::is_floating_point_v<T>);

        struct Weights final
        {
                Vector<2 * N + 1, T> mean;
                Vector<2 * N + 1, T> covariance;
        };

        static Weights create_weights(const T lambda, const T alpha, const T beta)
        {
                Weights res;
                res.mean[0] = lambda / (N + lambda);
                res.covariance[0] = res.mean[0] + 1 - square(alpha) + beta;
                for (std::size_t i = 1; i <= 2 * N; ++i)
                {
                        res.mean[i] = 1 / (2 * (N + lambda));
                        res.covariance[i] = res.mean[i];
                }
                return res;
        }

        T lambda_;
        Weights weights_;

public:
        SigmaPoints(
                const std::type_identity_t<T> alpha,
                const std::type_identity_t<T> beta,
                const std::type_identity_t<T> kappa)
                : lambda_(square(alpha) * (N + kappa) - N),
                  weights_(create_weights(lambda_, alpha, beta))
        {
        }

        [[nodiscard]] const Vector<2 * N + 1, T>& wm() const
        {
                return weights_.mean;
        }

        [[nodiscard]] const Vector<2 * N + 1, T>& wc() const
        {
                return weights_.covariance;
        }

        [[nodiscard]] std::array<Vector<N, T>, 2 * N + 1> points(const Vector<N, T>& x, const Matrix<N, N, T>& p) const
        {
                const Matrix<N, N, T> l = numerical::cholesky_decomposition_lower_triangular((N + lambda_) * p);

                std::array<Vector<N, T>, 2 * N + 1> res;
                res[0] = x;
                for (std::size_t i = 0; i < N; ++i)
                {
                        const Vector<N, T> c = l.column(i);
                        res[i + 1] = x + c;
                        res[i + N + 1] = x - c;
                }
                return res;
        }
};
}
