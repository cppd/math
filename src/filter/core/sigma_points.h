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
#include <cstddef>
#include <type_traits>

namespace ns::filter::core
{
template <std::size_t N, typename T>
class SigmaPoints final
{
        static_assert(std::is_floating_point_v<T>);

        struct Weights final
        {
                numerical::Vector<2 * N + 1, T> mean;
                numerical::Vector<2 * N + 1, T> covariance;
        };

        static long double lambda(const long double alpha, const long double kappa)
        {
                return square(alpha) * (N + kappa) - N;
        }

        static long double n_plus_lambda(const long double alpha, const long double kappa)
        {
                return square(alpha) * (N + kappa);
        }

        static Weights create_weights(const long double alpha, const long double beta, const long double kappa)
        {
                const long double l = lambda(alpha, kappa);
                const long double nl = n_plus_lambda(alpha, kappa);
                Weights res;
                res.mean[0] = l / nl;
                res.covariance[0] = l / nl + 1 - square(alpha) + beta;
                for (std::size_t i = 1; i <= 2 * N; ++i)
                {
                        res.mean[i] = 1 / (2 * nl);
                        res.covariance[i] = res.mean[i];
                }
                return res;
        }

        const T n_plus_lambda_;
        const Weights weights_;

public:
        struct Parameters final
        {
                T alpha;
                T beta;
                T kappa;
        };

        explicit SigmaPoints(const Parameters& parameters)
                : n_plus_lambda_(n_plus_lambda(parameters.alpha, parameters.kappa)),
                  weights_(create_weights(parameters.alpha, parameters.beta, parameters.kappa))
        {
        }

        [[nodiscard]] const numerical::Vector<2 * N + 1, T>& wm() const
        {
                return weights_.mean;
        }

        [[nodiscard]] const numerical::Vector<2 * N + 1, T>& wc() const
        {
                return weights_.covariance;
        }

        [[nodiscard]] std::array<numerical::Vector<N, T>, 2 * N + 1> points(
                const numerical::Vector<N, T>& x,
                const numerical::Matrix<N, N, T>& p) const
        {
                const numerical::Matrix<N, N, T> l =
                        numerical::cholesky_decomposition_lower_triangular(n_plus_lambda_ * p);

                std::array<numerical::Vector<N, T>, 2 * N + 1> res;
                res[0] = x;
                for (std::size_t i = 0; i < N; ++i)
                {
                        const numerical::Vector<N, T> c = l.column(i);
                        res[i + 1] = x + c;
                        res[i + N + 1] = x - c;
                }
                return res;
        }
};

template <std::size_t N, typename T>
[[nodiscard]] SigmaPoints<N, T> create_sigma_points(const std::type_identity_t<T> alpha)
{
        static constexpr T BETA = 2; // 2 for Gaussian
        static constexpr T KAPPA = 3 - T{N};

        return SigmaPoints<N, T>({
                .alpha = alpha,
                .beta = BETA,
                .kappa = KAPPA,
        });
}
}
