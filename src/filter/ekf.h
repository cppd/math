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
Roger R Labbe Jr.
Kalman and Bayesian Filters in Python.

6.9 The Kalman Filter Equations
7.4 Stable Compution of the Posterior Covariance
9.6 Detecting and Rejecting Bad Measurement
11.1 Linearizing the Kalman Filter
*/

/*
Dan Simon.
Optimal State Estimation. Kalman, H Infinity, and Nonlinear Approaches.
John Wiley & Sons, 2006.

5 The discrete-time Kalman filter
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
#include <string>

namespace ns::filter
{
namespace ekf_implementation
{
// Optimal State Estimation. Kalman, H Infinity, and Nonlinear Approaches.
// 11 The H infinity filter
// 11.89, 11.90
// Modern Spacecraft Guidance, Navigation, and Control.
// H infinity filter
template <std::size_t N, std::size_t M, typename T>
[[nodiscard]] Matrix<N, M, T> h_infinity_k(
        const char* const name,
        const T theta,
        const Matrix<N, N, T>& p,
        const Matrix<M, N, T>& h,
        const Matrix<N, M, T>& ht_ri)
{
        static constexpr Matrix<N, N, T> I = IDENTITY_MATRIX<N, T>;

        {
                const Matrix<N, N, T> m = p.inversed() - theta * I + ht_ri * h;
                if (!positive_definite(m))
                {
                        error(std::string(name) + ", H infinity condition is not hold\n"
                              + "matrix is not positive_definite\n" + to_string(m));
                }
        }

        return p * (I - theta * p + ht_ri * h * p).inversed() * ht_ri;
}
}

template <std::size_t N, typename T>
class Ekf final
{
        // State mean
        Vector<N, T> x_;

        // State covariance
        Matrix<N, N, T> p_;

public:
        Ekf(const Vector<N, T>& x, const Matrix<N, N, T>& p)
                : x_(x),
                  p_(p)
        {
                check_x_p("EKF constructor", x_, p_);
        }

        [[nodiscard]] const Vector<N, T>& x() const
        {
                return x_;
        }

        [[nodiscard]] const Matrix<N, N, T>& p() const
        {
                return p_;
        }

        template <typename F, typename FJ>
        void predict(
                // State transition function
                // Vector<N, T> f(const Vector<N, T>& x)
                const F f,
                // State transition function Jacobian
                // Matrix<N, N, T> f(const Vector<N, T>& x)
                const FJ fj,
                // Process covariance
                const Matrix<N, N, T>& q)
        {
                x_ = f(x_);

                const Matrix<N, N, T> fjx = fj(x_);
                p_ = fjx * p_ * fjx.transposed() + q;

                check_x_p("EKF predict", x_, p_);
        }

        template <std::size_t M, typename H, typename HJ, typename AddX, typename ResidualZ>
        UpdateInfo<M, T> update(
                // Measurement function
                // Vector<M, T> f(const Vector<N, T>& x)
                const H h,
                // Measurement function Jacobian
                // Matrix<M, N, T> f(const Vector<N, T>& x)
                const HJ hj,
                // Measurement covariance
                const Matrix<M, M, T>& r,
                // Measurement
                const Vector<M, T>& z,
                // The sum of the two state vectors
                // Vector<N, T> f(const Vector<N, T>& a, const Vector<N, T>& b)
                const AddX add_x,
                // The residual between the two measurement vectors
                // Vector<M, T> f(const Vector<M, T>& a, const Vector<M, T>& b)
                const ResidualZ residual_z,
                // H infinity parameter
                const std::optional<T> theta,
                // Mahalanobis distance gate
                const std::optional<T> gate,
                // compute normalized innovation
                const bool normalized_innovation,
                // compute likelihood
                const bool likelihood)
        {
                namespace impl = ekf_implementation;

                const Matrix<M, N, T> hjx = hj(x_);
                const Matrix<N, M, T> p_hjxt = p_ * hjx.transposed();

                struct Matrices final
                {
                        Matrix<M, M, T> s;
                        Matrix<M, M, T> s_inversed;
                };

                const std::optional<Matrices> matrices = [&]() -> std::optional<Matrices>
                {
                        if (!theta || gate || likelihood || normalized_innovation)
                        {
                                const Matrix<M, M, T> s = hjx * p_hjxt + r;
                                return {
                                        {.s = s, .s_inversed = s.inversed()}
                                };
                        }
                        return {};
                }();

                const Vector<M, T> residual = residual_z(z, h(x_));

                const UpdateInfo<M, T> res = [&]()
                {
                        if (gate || likelihood || normalized_innovation)
                        {
                                ASSERT(matrices);
                                return make_update_info(
                                        residual, matrices->s, matrices->s_inversed, gate, likelihood,
                                        normalized_innovation);
                        }
                        return make_update_info(residual);
                }();

                if (res.gate)
                {
                        return res;
                }

                const Matrix<N, M, T> k = [&]()
                {
                        if (!theta)
                        {
                                ASSERT(matrices);
                                return p_hjxt * matrices->s_inversed;
                        }
                        const Matrix<N, M, T> ht_ri = hjx.transposed() * r.inversed();
                        return impl::h_infinity_k("EKF update", *theta, p_, hjx, ht_ri);
                }();

                x_ = add_x(x_, k * residual);

                const Matrix<N, N, T> i_kh = IDENTITY_MATRIX<N, T> - k * hjx;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();

                check_x_p("EKF update", x_, p_);

                return res;
        }
};
}
