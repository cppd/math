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
template <std::size_t N, typename T>
class Ekf final
{
        // state mean
        Vector<N, T> x_;

        // state covariance
        Matrix<N, N, T> p_;

public:
        Ekf(const Vector<N, T>& x /*state mean*/, const Matrix<N, N, T>& p /*state covariance*/)
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
                const Matrix<N, N, T>& f /*state transition function*/,
                const Matrix<N, N, T>& f_t /*state transition function transposed*/,
                const Matrix<N, N, T>& q /*process covariance*/)
        {
                x_ = f * x_;
                p_ = f * p_ * f_t + q;
        }

        template <typename F, typename FJ>
        void predict(
                const F& f /*state transition function*/,
                const FJ& fj /*state transition function Jacobian*/,
                const Matrix<N, N, T>& q /*process covariance*/)
        {
                x_ = f(x_);

                const Matrix<N, N, T> fjx = fj(x_);
                p_ = fjx * p_ * fjx.transposed() + q;
        }

        template <std::size_t M>
        void update(
                const Matrix<M, N, T>& h /*measurement function*/,
                const Matrix<N, M, T>& h_t /*measurement function transposed*/,
                const Matrix<M, M, T>& r /* measurement covariance*/,
                const Vector<M, T>& z /*measurement*/)
        {
                const Matrix<N, M, T> k = p_ * h_t * (h * p_ * h_t + r).inversed();
                x_ = x_ + k * (z - h * x_);

                const Matrix<N, N, T> i_kh = IDENTITY_MATRIX<N, T> - k * h;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();
        }

        template <std::size_t M, typename H, typename HJ>
        void update(
                const H& h /*measurement function*/,
                const HJ& hj /*measurement function Jacobian*/,
                const Matrix<M, M, T>& r /* measurement covariance*/,
                const Vector<M, T>& z /*measurement*/)
        {
                const Matrix<M, N, T> hjx = hj(x_);
                const Matrix<N, M, T> hjx_t = hjx.transposed();

                const Matrix<N, M, T> k = p_ * hjx_t * (hjx * p_ * hjx_t + r).inversed();
                x_ = x_ + k * (z - h(x_));

                const Matrix<N, N, T> i_kh = IDENTITY_MATRIX<N, T> - k * hjx;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();
        }
};
}
