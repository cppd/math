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

#pragma once

#include <src/numerical/matrix.h>

#include <cstddef>
#include <type_traits>

namespace ns::filter::core
{
template <std::size_t N, typename T>
        requires (N == 1)
[[nodiscard]] constexpr numerical::Matrix<N, N, T> continuous_white_noise(
        const std::type_identity_t<T> dt,
        const std::type_identity_t<T> spectral_density)
{
        static_assert(std::is_floating_point_v<T>);

        return numerical::Matrix<N, N, T>{{dt * spectral_density}};
}

// 6.2.2 Continuous White Noise Acceleration Model
// The changes in the velocity over a sampling period T are
// of the order of sqrt(Q(2, 2)) = sqrt(spectral_density * T).
template <std::size_t N, typename T>
        requires (N == 2)
[[nodiscard]] constexpr numerical::Matrix<N, N, T> continuous_white_noise(
        const std::type_identity_t<T> dt,
        const std::type_identity_t<T> spectral_density)
{
        static_assert(std::is_floating_point_v<T>);

        const T dt_1 = dt * spectral_density;
        const T dt_2 = dt_1 * dt;
        const T dt_3 = dt_2 * dt;

        return numerical::Matrix<N, N, T>{
                {dt_3 / 3, dt_2 / 2},
                {dt_2 / 2,     dt_1}
        };
}

// 6.2.3 Continuous Wiener Process Acceleration Model
// The changes in the acceleration over a sampling period T are
// of the order of sqrt(Q(3, 3)) = sqrt(spectral_density * T).
template <std::size_t N, typename T>
        requires (N == 3)
[[nodiscard]] constexpr numerical::Matrix<N, N, T> continuous_white_noise(
        const std::type_identity_t<T> dt,
        const std::type_identity_t<T> spectral_density)
{
        static_assert(std::is_floating_point_v<T>);

        const T dt_1 = dt * spectral_density;
        const T dt_2 = dt_1 * dt;
        const T dt_3 = dt_2 * dt;
        const T dt_4 = dt_3 * dt;
        const T dt_5 = dt_4 * dt;

        return numerical::Matrix<N, N, T>{
                {dt_5 / 20, dt_4 / 8, dt_3 / 6},
                { dt_4 / 8, dt_3 / 3, dt_2 / 2},
                { dt_3 / 6, dt_2 / 2,     dt_1}
        };
}

template <std::size_t N, typename T>
        requires (N == 1)
[[nodiscard]] constexpr numerical::Matrix<N, N, T> discrete_white_noise(
        const std::type_identity_t<T> dt,
        const std::type_identity_t<T> variance)
{
        static_assert(std::is_floating_point_v<T>);

        return numerical::Matrix<N, N, T>{{dt * dt * variance}};
}

// 6.3.2 Discrete White Noise Acceleration Model
// For this model, standard deviation (sigma = sqrt(variance))
// should be of the order of the maximum acceleration magnitude (a).
// A practical range is a/2 <= sigma <= a.
template <std::size_t N, typename T>
        requires (N == 2)
[[nodiscard]] constexpr numerical::Matrix<N, N, T> discrete_white_noise(
        const std::type_identity_t<T> dt,
        const std::type_identity_t<T> variance)
{
        static_assert(std::is_floating_point_v<T>);

        const T dt_2 = dt * dt * variance;
        const T dt_3 = dt_2 * dt;
        const T dt_4 = dt_3 * dt;

        return numerical::Matrix<N, N, T>{
                {dt_4 / 4, dt_3 / 2},
                {dt_3 / 2,     dt_2}
        };
}

// 6.3.3 Discrete Wiener Process Acceleration Model
// For this model, standard deviation (sigma = sqrt(variance))
// should be of the order of the magnitude of the maximum
// acceleration increment over a sampling period (d).
// A practical range is d/2 <= sigma <= d.
template <std::size_t N, typename T>
        requires (N == 3)
[[nodiscard]] constexpr numerical::Matrix<N, N, T> discrete_white_noise(
        const std::type_identity_t<T> dt,
        const std::type_identity_t<T> variance)
{
        static_assert(std::is_floating_point_v<T>);

        const T dt_0 = variance;
        const T dt_1 = dt_0 * dt;
        const T dt_2 = dt_1 * dt;
        const T dt_3 = dt_2 * dt;
        const T dt_4 = dt_3 * dt;

        return numerical::Matrix<N, N, T>{
                {dt_4 / 4, dt_3 / 2, dt_2 / 2},
                {dt_3 / 2,     dt_2,     dt_1},
                {dt_2 / 2,     dt_1,     dt_0}
        };
}
}
