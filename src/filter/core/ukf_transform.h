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

10.4 The Unscented Transform
10.5 The Unscented Kalman Filter
10.11 Implementation of the UKF
*/

#pragma once

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <tuple>

namespace ns::filter::core
{
template <std::size_t N, typename T, std::size_t COUNT, typename NoiseCovariance>
[[nodiscard]] std::tuple<numerical::Vector<N, T>, numerical::Matrix<N, N, T>> unscented_transform(
        const std::array<numerical::Vector<N, T>, COUNT>& points,
        const numerical::Vector<COUNT, T>& wm,
        const numerical::Vector<COUNT, T>& wc,
        const NoiseCovariance& noise_covariance,
        const T fading_memory_alpha = 1)
{
        static_assert(
                std::is_same_v<NoiseCovariance, numerical::Matrix<N, N, T>>
                || std::is_same_v<NoiseCovariance, numerical::Vector<N, T>>);

        const numerical::Vector<N, T> mean = [&]
        {
                static_assert(COUNT > 0);
                numerical::Vector<N, T> res = points[0] * wm[0];
                for (std::size_t i = 1; i < COUNT; ++i)
                {
                        res.multiply_add(points[i], wm[i]);
                }
                return res;
        }();

        numerical::Matrix<N, N, T> covariance(numerical::ZERO_MATRIX);
        for (std::size_t i = 0; i < COUNT; ++i)
        {
                const numerical::Vector<N, T> v = points[i] - mean;
                for (std::size_t r = 0; r < N; ++r)
                {
                        for (std::size_t c = 0; c < N; ++c)
                        {
                                covariance[r, c] += wc[i] * v[r] * v[c];
                        }
                }
        }

        if (fading_memory_alpha == 1)
        {
                return {mean, numerical::add_md(covariance, noise_covariance)};
        }

        const T factor = square(fading_memory_alpha);
        return {mean, numerical::add_md(factor * covariance, noise_covariance)};
}

template <std::size_t N, std::size_t M, typename T, std::size_t COUNT>
[[nodiscard]] numerical::Matrix<N, M, T> cross_covariance(
        const numerical::Vector<COUNT, T>& wc,
        const std::array<numerical::Vector<N, T>, COUNT>& sigmas_f,
        const numerical::Vector<N, T>& x,
        const std::array<numerical::Vector<M, T>, COUNT>& sigmas_h,
        const numerical::Vector<M, T>& z)
{
        numerical::Matrix<N, M, T> res(numerical::ZERO_MATRIX);
        for (std::size_t i = 0; i < COUNT; ++i)
        {
                const numerical::Vector<N, T> vr = sigmas_f[i] - x;
                const numerical::Vector<M, T> vc = sigmas_h[i] - z;
                for (std::size_t r = 0; r < N; ++r)
                {
                        for (std::size_t c = 0; c < M; ++c)
                        {
                                res[r, c] += wc[i] * vr[r] * vc[c];
                        }
                }
        }
        return res;
}
}
