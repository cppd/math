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

#pragma once

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

namespace ns::filter
{
namespace ekf_implementation
{
struct Residual final
{
        template <std::size_t N, typename T>
        [[nodiscard]] Vector<N, T> operator()(const Vector<N, T>& a, const Vector<N, T>& b) const
        {
                return a - b;
        }
};

struct Add final
{
        template <std::size_t N, typename T>
        [[nodiscard]] Vector<N, T> operator()(const Vector<N, T>& a, const Vector<N, T>& b) const
        {
                return a + b;
        }
};
}

template <std::size_t N, typename T>
class Ekf final
{
        // state mean
        Vector<N, T> x_;

        // state covariance
        Matrix<N, N, T> p_;

public:
        Ekf(const Vector<N, T>& x, const Matrix<N, N, T>& p)
                : x_(x),
                  p_(p)
        {
        }

        [[nodiscard]] const Vector<N, T>& x() const
        {
                return x_;
        }

        [[nodiscard]] const Matrix<N, N, T>& p() const
        {
                return p_;
        }

        void predict(
                // State transition function
                const Matrix<N, N, T>& f,
                // State transition function transposed
                const Matrix<N, N, T>& f_t,
                // Process covariance
                const Matrix<N, N, T>& q)
        {
                x_ = f * x_;
                p_ = f * p_ * f_t + q;
        }

        template <typename F, typename FJ>
        void predict(
                // State transition function
                // Vector<N, T> operator()(
                //   const Vector<N, T>& x) const
                const F f,
                // State transition function Jacobian
                // Matrix<N, N, T> operator()(
                //   const Vector<N, T>& x) const
                const FJ fj,
                // Process covariance
                const Matrix<N, N, T>& q)
        {
                x_ = f(x_);

                const Matrix<N, N, T> fjx = fj(x_);
                p_ = fjx * p_ * fjx.transposed() + q;
        }

        template <std::size_t M>
        void update(
                // Measurement function
                const Matrix<M, N, T>& h,
                // Measurement function transposed
                const Matrix<N, M, T>& h_t,
                // measurement covariance
                const Matrix<M, M, T>& r,
                // Measurement
                const Vector<M, T>& z)
        {
                const Matrix<N, M, T> k = p_ * h_t * (h * p_ * h_t + r).inversed();
                x_ = x_ + k * (z - h * x_);

                const Matrix<N, N, T> i_kh = IDENTITY_MATRIX<N, T> - k * h;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();
        }

        template <std::size_t M, typename H, typename HJ, typename ResidualZ, typename AddX>
        void update(
                // Measurement function
                // Vector<M, T> operator()(
                //   const Vector<N, T>& x) const
                const H h,
                // Measurement function Jacobian
                // Matrix<M, N, T> operator()(
                //   const Vector<N, T>& x) const
                const HJ hj,
                // Measurement covariance
                const Matrix<M, M, T>& r,
                // measurement
                const Vector<M, T>& z,
                // The sum of the two state vectors
                // Vector<N, T> operator()(
                //   const Vector<N, T>& a,
                //   const Vector<N, T>& b) const
                const AddX add_x,
                // The residual between the two measurement vectors
                // Vector<M, T> operator()(
                //   const Vector<M, T>& a,
                //   const Vector<M, T>& b) const
                const ResidualZ residual_z)
        {
                const Matrix<M, N, T> hjx = hj(x_);
                const Matrix<N, M, T> hjx_t = hjx.transposed();

                const Matrix<N, M, T> k = p_ * hjx_t * (hjx * p_ * hjx_t + r).inversed();
                x_ = add_x(x_, k * residual_z(z, h(x_)));

                const Matrix<N, N, T> i_kh = IDENTITY_MATRIX<N, T> - k * hjx;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();
        }
};
}
