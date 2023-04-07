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
Yaakov Bar-Shalom, X.-Rong Li, Thiagalingam Kirubarajan.
Estimation with Applications To Tracking and Navigation.
John Wiley & Sons, 2001.

6.2 DISCRETIZED CONTINUOUS-TIME KINEMATIC MODELS
6.3 DIRECT DISCRETE-TIME KINEMATIC MODELS
*/

/*
Roger R Labbe Jr.
Kalman and Bayesian Filters in Python.

7.3.1 Continuous White Noise Model
7.3.2 Piecewise White Noise Model
*/

#include "models.h"

#include <src/com/exponent.h>

namespace ns::filter
{
// 6.2.2 Continuous White Noise Acceleration Model
// The changes in the velocity over a sampling period T are
// of the order of sqrt(Q(2, 2)) = sqrt(spectral_density * T).
template <std::size_t N, typename T>
        requires (N == 2)
Matrix<N, N, T> continuous_white_noise(const T dt, const T spectral_density)
{
        const T dt_2 = power<2>(dt);
        const T dt_3 = power<3>(dt);
        const T sd = spectral_density;

        // clang-format off
        return Matrix<N, N, T>
        {{{
                {dt_3 / 3 * sd, dt_2 / 2 * sd},
                {dt_2 / 2 * sd,       dt * sd}
        }}};
        // clang-format on
}

// 6.2.3 Continuous Wiener Process Acceleration Model
// The changes in the acceleration over a sampling period T are
// of the order of sqrt(Q(3, 3)) = sqrt(spectral_density * T).
template <std::size_t N, typename T>
        requires (N == 3)
Matrix<N, N, T> continuous_white_noise(const T dt, const T spectral_density)
{
        const T dt_2 = power<2>(dt);
        const T dt_3 = power<3>(dt);
        const T dt_4 = power<4>(dt);
        const T dt_5 = power<5>(dt);
        const T sd = spectral_density;

        // clang-format off
        return Matrix<N, N, T>
        {{{
                {dt_5 / 20 * sd, dt_4 / 8 * sd, dt_3 / 6 * sd},
                {dt_4 /  8 * sd, dt_3 / 3 * sd, dt_2 / 2 * sd},
                {dt_3 /  6 * sd, dt_2 / 2 * sd,       dt * sd}
        }}};
        // clang-format on
}

// 6.3.2 Discrete White Noise Acceleration Model
// For this model, standard deviation (sigma = sqrt(variance))
// should be of the order of the maximum acceleration magnitude (a).
// A practical range is a/2 <= sigma <= a.
template <std::size_t N, typename T>
        requires (N == 2)
Matrix<N, N, T> discrete_white_noise(const T dt, const T variance)
{
        const T dt_2 = power<2>(dt);
        const T dt_3 = power<3>(dt);
        const T dt_4 = power<4>(dt);
        const T v = variance;

        // clang-format off
        return Matrix<N, N, T>
        {{{
                {dt_4 / 4 * v, dt_3 / 2 * v},
                {dt_3 / 2 * v,     dt_2 * v}
        }}};
        // clang-format on
}

// 6.3.3 Discrete Wiener Process Acceleration Model
// For this model, standard deviation (sigma = sqrt(variance))
// should be of the order of the magnitude of the maximum
// acceleration increment over a sampling period (d).
// A practical range is d/2 <= sigma <= d.
template <std::size_t N, typename T>
        requires (N == 3)
Matrix<N, N, T> discrete_white_noise(const T dt, const T variance)
{
        const T dt_2 = power<2>(dt);
        const T dt_3 = power<3>(dt);
        const T dt_4 = power<4>(dt);
        const T v = variance;

        // clang-format off
        return Matrix<N, N, T>
        {{{
                {dt_4 / 4 * v, dt_3 / 2 * v, dt_2 / 2 * v},
                {dt_3 / 2 * v,     dt_2 * v,       dt * v},
                {dt_2 / 2 * v,       dt * v,        1 * v}
        }}};
        // clang-format on
}

#define INSTANTIATION(T)                                       \
        template Matrix<2, 2, T> continuous_white_noise(T, T); \
        template Matrix<3, 3, T> continuous_white_noise(T, T); \
        template Matrix<2, 2, T> discrete_white_noise(T, T);   \
        template Matrix<3, 3, T> discrete_white_noise(T, T);

INSTANTIATION(float)
INSTANTIATION(double)
INSTANTIATION(long double)
}
