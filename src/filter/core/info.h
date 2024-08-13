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
Dan Simon.
Optimal State Estimation. Kalman, H Infinity, and Nonlinear Approaches.
John Wiley & Sons, 2006.

6.2 Information filtering
7.4 Kalman filtering with fading memory
*/

/*
Yaakov Bar-Shalom, X.-Rong Li, Thiagalingam Kirubarajan.
Estimation with Applications To Tracking and Navigation.
John Wiley & Sons, 2001.

7.2 THE INFORMATION FILTER
*/

#pragma once

#include "update_info.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <cstddef>
#include <optional>

namespace ns::filter::core
{
template <std::size_t N, typename T>
class Info final
{
        // State mean
        numerical::Vector<N, T> x_;

        // State information
        numerical::Matrix<N, N, T> i_;

        // State covariance
        std::optional<numerical::Matrix<N, N, T>> p_;

public:
        Info(const numerical::Vector<N, T>& x, const numerical::Matrix<N, N, T>& i)
                : x_(x),
                  i_(i)
        {
        }

        [[nodiscard]] const numerical::Vector<N, T>& x() const
        {
                return x_;
        }

        [[nodiscard]] const numerical::Matrix<N, N, T>& i() const
        {
                return i_;
        }

        [[nodiscard]] const std::optional<numerical::Matrix<N, N, T>>& p() const
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
                // Process covariance inversed
                const numerical::Matrix<N, N, T>& q_inv,
                // Fading memory alpha
                const T fading_memory_alpha = 1)
        {
                x_ = f(x_);

                const numerical::Matrix<N, N, T> fjx = fj(x_);
                const numerical::Matrix<N, N, T> fjx_t = fjx.transposed();

                if (!(fading_memory_alpha == 1))
                {
                        const T factor = 1 / square(fading_memory_alpha);
                        i_ = factor * i_;
                }

                i_ = q_inv - q_inv * fjx * (i_ + fjx_t * q_inv * fjx).inversed() * fjx_t * q_inv;
        }

        template <std::size_t M, typename H, typename HJ, typename AddX, typename ResidualZ>
        UpdateInfo<M, T> update(
                // Measurement function
                // numerical::Vector<M, T> f(const numerical::Vector<N, T>& x)
                const H h,
                // Measurement function Jacobian
                // numerical::Matrix<M, N, T> f(const numerical::Vector<N, T>& x)
                const HJ hj,
                // Measurement covariance inversed
                const numerical::Matrix<M, M, T>& r_inv,
                // Measurement
                const numerical::Vector<M, T>& z,
                // The sum of the two state vectors
                // numerical::Vector<N, T> f(const numerical::Vector<N, T>& a, const numerical::Vector<N, T>& b)
                const AddX add_x,
                // The residual between the two measurement vectors
                // numerical::Vector<M, T> f(const numerical::Vector<M, T>& a, const numerical::Vector<M, T>& b)
                const ResidualZ residual_z,
                // Mahalanobis distance gate
                const std::optional<T> gate)
        {
                const numerical::Matrix<M, N, T> hjx = hj(x_);
                const numerical::Matrix<N, M, T> hjx_t = hjx.transposed();

                const numerical::Vector<M, T> residual = residual_z(z, h(x_));

                const UpdateInfo<M, T> res = [&]
                {
                        if (gate)
                        {
                                const numerical::Matrix<M, M, T> s_inv =
                                        r_inv - r_inv * hjx * (i_ + hjx_t * r_inv * hjx).inversed() * hjx_t * r_inv;
                                return make_update_info(residual, s_inv, gate);
                        }
                        return make_update_info(residual);
                }();

                if (res.gate)
                {
                        return res;
                }

                i_ = i_ + hjx_t * r_inv * hjx;
                p_ = i_.inversed();

                const numerical::Matrix<N, M, T> k = *p_ * hjx_t * r_inv;

                x_ = add_x(x_, k * residual);

                return res;
        }
};
}
