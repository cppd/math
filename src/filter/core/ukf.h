/*
Copyright (C) 2017-2026 Topological Manifold

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

9.6 Detecting and Rejecting Bad Measurement
10.4 The Unscented Transform
10.5 The Unscented Kalman Filter
10.11 Implementation of the UKF
14.5 Fading Memory Filter
*/

/*
Dan Simon.
Optimal State Estimation. Kalman, H Infinity, and Nonlinear Approaches.
John Wiley & Sons, 2006.

7.4 Kalman filtering with fading memory
*/

#pragma once

#include "checks.h"
#include "ukf_transform.h"
#include "update_info.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <optional>
#include <tuple>
#include <type_traits>

namespace ns::filter::core
{
namespace ukf_implementation
{
template <std::size_t N, typename T, typename F, std::size_t COUNT>
[[nodiscard]] auto apply(const F f, const std::array<numerical::Vector<N, T>, COUNT>& points)
{
        std::array<std::remove_cvref_t<decltype(f(points[0]))>, COUNT> res;
        for (std::size_t i = 0; i < COUNT; ++i)
        {
                res[i] = f(points[i]);
        }
        return res;
}
}

template <std::size_t N, typename T, typename SigmaPoints>
class Ukf final
{
        static constexpr std::size_t POINT_COUNT =
                std::tuple_size_v<std::remove_cvref_t<decltype(std::declval<SigmaPoints>().wm())>>;

        static_assert(POINT_COUNT >= 2 * N + 1);

        SigmaPoints sigma_points_;

        std::array<numerical::Vector<N, T>, POINT_COUNT> sigmas_f_;

        // State mean
        numerical::Vector<N, T> x_;

        // State covariance
        numerical::Matrix<N, N, T> p_;

public:
        Ukf(SigmaPoints sigma_points, const numerical::Vector<N, T>& x, const numerical::Matrix<N, N, T>& p)
                : sigma_points_(std::move(sigma_points)),
                  x_(x),
                  p_(p)
        {
                check_x_p("UKF constructor", x_, p_);
        }

        [[nodiscard]] const numerical::Vector<N, T>& x() const
        {
                return x_;
        }

        [[nodiscard]] const numerical::Matrix<N, N, T>& p() const
        {
                return p_;
        }

        template <typename F>
        void predict(
                // State transition function
                // numerical::Vector<N, T> f(const numerical::Vector<N, T>& x)
                const F f,
                // Process covariance
                const numerical::Matrix<N, N, T>& q,
                // Fading memory alpha
                const T fading_memory_alpha = 1)
        {
                ASSERT(fading_memory_alpha >= 1);

                namespace impl = ukf_implementation;

                sigmas_f_ = impl::apply(f, sigma_points_.points(x_, p_));

                std::tie(x_, p_) =
                        unscented_transform(sigmas_f_, sigma_points_.wm(), sigma_points_.wc(), q, fading_memory_alpha);

                check_x_p("UKF predict", x_, p_);
        }

        template <std::size_t M, typename H, typename AddX, typename ResidualZ>
        UpdateInfo<M, T> update(
                // Measurement function
                // numerical::Vector<M, T> f(const numerical::Vector<N, T>& x)
                const H h,
                // Measurement covariance
                const numerical::Matrix<M, M, T>& r,
                // Measurement
                const numerical::Vector<M, T>& z,
                // The sum of the two state vectors
                // numerical::Vector<N, T> f(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b)
                const AddX add_x,
                // The residual between the two measurement vectors
                // numerical::Vector<M, T> f(const numerical::Vector<M, T>& a, const numerical::Vector<M, T>& b)
                const ResidualZ residual_z,
                // Mahalanobis distance gate
                const std::optional<T> gate,
                // compute normalized innovation
                const bool normalized_innovation,
                // compute likelihood
                const bool likelihood)
        {
                namespace impl = ukf_implementation;

                const std::array<numerical::Vector<M, T>, POINT_COUNT> sigmas_h = impl::apply(h, sigmas_f_);

                const auto [x_z, p_z] = unscented_transform(sigmas_h, sigma_points_.wm(), sigma_points_.wc(), r);

                check_x_p("UKF update measurement", x_z, p_z);

                const numerical::Matrix<N, M, T> p_xz =
                        cross_covariance(sigma_points_.wc(), sigmas_f_, x_, sigmas_h, x_z);

                const numerical::Matrix<M, M, T> p_z_inversed = p_z.inversed();
                const numerical::Vector<M, T> residual = residual_z(z, x_z);

                const UpdateInfo<M, T> res =
                        make_update_info(residual, p_z, p_z_inversed, gate, likelihood, normalized_innovation);

                if (res.gate)
                {
                        return res;
                }

                const numerical::Matrix<N, M, T> k = p_xz * p_z_inversed;

                x_ = add_x(x_, k * residual);
                p_ = p_ - p_xz * k.transposed();

                check_x_p("UKF update", x_, p_);

                return res;
        }
};
}
