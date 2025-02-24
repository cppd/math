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

6.9 The Kalman Filter Equations
7.4 Stable Compution of the Posterior Covariance
9.6 Detecting and Rejecting Bad Measurement
11.1 Linearizing the Kalman Filter
14.5 Fading Memory Filter
*/

/*
Dan Simon.
Optimal State Estimation. Kalman, H Infinity, and Nonlinear Approaches.
John Wiley & Sons, 2006.

5 The discrete-time Kalman filter
7.4 Kalman filtering with fading memory
11 The H infinity filter
*/

/*
Edited by Vincenzo Pesce, Andrea Colagrossi, Stefano Silvestrini.
Modern Spacecraft Guidance, Navigation, and Control.
Elsevier, 2023.

9 Navigation
Sequential filters
*/

#pragma once

#include "checks.h"
#include "update_info.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>

namespace ns::filter::core
{
namespace ekf_implementation
{
// Optimal State Estimation. Kalman, H Infinity, and Nonlinear Approaches.
// 11 The H infinity filter
// 11.89, 11.90
// Modern Spacecraft Guidance, Navigation, and Control.
// H infinity filter
template <std::size_t N, std::size_t M, typename T>
[[nodiscard]] numerical::Matrix<N, M, T> h_infinity_k(
        const T theta,
        const numerical::Matrix<N, N, T>& p,
        const numerical::Matrix<M, N, T>& h,
        const numerical::Matrix<N, M, T>& ht_ri)
{
        static constexpr numerical::Matrix<N, N, T> I = numerical::IDENTITY_MATRIX<N, T>;

        {
                const numerical::Matrix<N, N, T> m = p.inversed() - theta * I + ht_ri * h;
                if (!positive_definite(m))
                {
                        error("H infinity condition is not hold\n"
                              "matrix is not positive_definite\n"
                              + to_string(m));
                }
        }

        return p * (I - theta * p + ht_ri * h * p).inversed() * ht_ri;
}
}

template <std::size_t N, typename T>
class Ekf final
{
        // State mean
        numerical::Vector<N, T> x_;

        // State covariance
        numerical::Matrix<N, N, T> p_;

public:
        Ekf(const numerical::Vector<N, T>& x, const numerical::Matrix<N, N, T>& p)
                : x_(x),
                  p_(p)
        {
                check_x_p("EKF constructor", x_, p_);
        }

        [[nodiscard]] const numerical::Vector<N, T>& x() const
        {
                return x_;
        }

        [[nodiscard]] const numerical::Matrix<N, N, T>& p() const
        {
                return p_;
        }

        template <typename F, typename FJ>
        void predict(
                // State transition function
                // numerical::Vector<N, T> f(const numerical::Vector<N, T>& x)
                const F f,
                // State transition function Jacobian
                // numerical::Matrix<N, N, T> f(const numerical::Vector<N, T>& x)
                const FJ fj,
                // Process covariance
                const numerical::Matrix<N, N, T>& q,
                // Fading memory alpha
                const T fading_memory_alpha = 1)
        {
                ASSERT(fading_memory_alpha >= 1);

                x_ = f(x_);

                const numerical::Matrix<N, N, T> fjx = fj(x_);
                const numerical::Matrix<N, N, T> covariance = fjx * p_ * fjx.transposed();

                if (fading_memory_alpha == 1)
                {
                        p_ = covariance + q;
                }
                else
                {
                        const T factor = square(fading_memory_alpha);
                        p_ = factor * covariance + q;
                }

                check_x_p("EKF predict", x_, p_);
        }

        template <std::size_t M, typename H, typename HJ, typename AddX, typename ResidualZ>
        UpdateInfo<M, T> update(
                // Measurement function
                // numerical::Vector<M, T> f(const numerical::Vector<N, T>& x)
                const H h,
                // Measurement function Jacobian
                // numerical::Matrix<M, N, T> f(const numerical::Vector<N, T>& x)
                const HJ hj,
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
                const numerical::Matrix<M, N, T> hjx = hj(x_);
                const numerical::Matrix<N, M, T> p_hjxt = p_ * hjx.transposed();

                const numerical::Matrix<M, M, T> s = hjx * p_hjxt + r;
                const numerical::Matrix<M, M, T> s_inv = s.inversed();

                const numerical::Vector<M, T> residual = residual_z(z, h(x_));

                const UpdateInfo<M, T> res =
                        make_update_info(residual, s, s_inv, gate, likelihood, normalized_innovation);

                if (res.gate)
                {
                        return res;
                }

                const numerical::Matrix<N, M, T> k = p_hjxt * s_inv;

                x_ = add_x(x_, k * residual);

                const numerical::Matrix<N, N, T> i_kh = numerical::IDENTITY_MATRIX<N, T> - k * hjx;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();

                check_x_p("EKF update", x_, p_);

                return res;
        }

        template <std::size_t M, typename H, typename HJ, typename AddX, typename ResidualZ>
        UpdateInfo<M, T> update(
                // Measurement function
                // numerical::Vector<M, T> f(const numerical::Vector<N, T>& x)
                const H h,
                // Measurement function Jacobian
                // numerical::Matrix<M, N, T> f(const numerical::Vector<N, T>& x)
                const HJ hj,
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
                const bool likelihood,
                // H infinity parameter
                const T theta)
        {
                namespace impl = ekf_implementation;

                const numerical::Matrix<M, N, T> hjx = hj(x_);
                const numerical::Matrix<N, M, T> hjxt = hjx.transposed();

                const numerical::Vector<M, T> residual = residual_z(z, h(x_));

                const UpdateInfo<M, T> res = [&]
                {
                        if (gate || likelihood || normalized_innovation)
                        {
                                const numerical::Matrix<M, M, T> s = hjx * p_ * hjxt + r;
                                const numerical::Matrix<M, M, T> s_inv = s.inversed();
                                return make_update_info(residual, s, s_inv, gate, likelihood, normalized_innovation);
                        }
                        return make_update_info(residual);
                }();

                if (res.gate)
                {
                        return res;
                }

                const numerical::Matrix<N, M, T> k = impl::h_infinity_k(theta, p_, hjx, hjxt * r.inversed());

                x_ = add_x(x_, k * residual);

                const numerical::Matrix<N, N, T> i_kh = numerical::IDENTITY_MATRIX<N, T> - k * hjx;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();

                check_x_p("EKF H INFINITY update", x_, p_);

                return res;
        }
};
}
