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

namespace ns::filter
{
template <std::size_t N, typename T>
class Filter final
{
        // state mean
        Vector<N, T> x_;

        // state covariance
        Matrix<N, N, T> p_;

        // state transition function
        Matrix<N, N, T> f_;
        Matrix<N, N, T> f_t_; // transposed

        // process covariance
        Matrix<N, N, T> q_;

public:
        // state mean
        void set_x(const Vector<N, T>& x)
        {
                x_ = x;
        }

        // state covariance
        void set_p(const Matrix<N, N, T>& p)
        {
                p_ = p;
        }

        // state transition function
        void set_f(const Matrix<N, N, T>& f)
        {
                f_ = f;
                f_t_ = f_.transpose();
        }

        // process covariance
        void set_q(const Matrix<N, N, T>& q)
        {
                q_ = q;
        }

        [[nodiscard]] const Vector<N, T>& x() const
        {
                return x_;
        }

        [[nodiscard]] const Matrix<N, N, T>& p() const
        {
                return p_;
        }

        void predict()
        {
                x_ = f_ * x_;
                p_ = f_ * p_ * f_t_ + q_;
        }

        template <std::size_t M>
        void update(
                const Matrix<M, N, T>& h /*measurement function*/,
                const Matrix<N, M, T>& h_t /*measurement function transposed*/,
                const Matrix<M, M, T>& r /* measurement covariance*/,
                const Vector<M, T>& z /*measurement*/)
        {
                const Matrix<N, M, T> k = p_ * h_t * (h * p_ * h_t + r).inverse();
                x_ = x_ + k * (z - h * x_);

                const Matrix<N, N, T> i_kh = IDENTITY_MATRIX<N, T> - k * h;
                p_ = i_kh * p_ * i_kh.transpose() + k * r * k.transpose();
        }
};
}
