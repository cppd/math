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
template <std::size_t N, std::size_t M, typename T>
class Filter final
{
        static constexpr Matrix<N, N, T> I{1};

        // state mean
        Vector<N, T> x_;

        // state covariance
        Matrix<N, N, T> p_;

        // state transition function
        Matrix<N, N, T> f_;
        Matrix<N, N, T> f_t_; // transposed

        // process covariance
        Matrix<N, N, T> q_;

        // measurement function
        Matrix<M, N, T> h_;
        Matrix<N, M, T> h_t_; // transposed

        // measurement covariance
        Matrix<M, M, T> r_;

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

        // measurement function
        void set_h(const Matrix<M, N, T>& h)
        {
                h_ = h;
                h_t_ = h_.transpose();
        }

        // measurement covariance
        void set_r(const Matrix<M, M, T>& r)
        {
                r_ = r;
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

        void update(const Vector<M, T>& z) // measurement
        {
                const Matrix<N, M, T> k = p_ * h_t_ * (h_ * p_ * h_t_ + r_).inverse();
                x_ = x_ + k * (z - h_ * x_);

                const Matrix<N, N, T> i_kh = I - k * h_;
                p_ = i_kh * p_ * i_kh.transpose() + k * r_ * k.transpose();
        }
};
}
