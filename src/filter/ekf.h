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

/*
Roger R Labbe Jr.
Kalman and Bayesian Filters in Python.

6.9 The Kalman Filter Equations
7.4 Stable Compution of the Posterior Covariance
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

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

namespace ns::filter
{
namespace ekf_implementation
{
template <std::size_t N, typename T>
[[nodiscard]] bool positive_definite(const Matrix<N, N, T>& p)
{
        // this is insufficient check based on diagonal only
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(p(i, i) > 0))
                {
                        return false;
                }
        }
        return true;
}

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

        const Matrix<N, N, T> m = p.inversed() - theta * I + ht_ri * h;
        if (!positive_definite(m))
        {
                error(std::string(name) + ", H infinity condition is not hold\n" + to_string(m));
        }
        return p * (I - theta * p + ht_ri * h * p).inversed() * ht_ri;
}

template <std::size_t N, typename T>
void check_x_p(const char* const name, const Vector<N, T>& x, const Matrix<N, N, T>& p)
{
        if (!positive_definite(p))
        {
                error(std::string(name) + ", P is not positive definite\nx\n" + to_string(x) + "\nP\n" + to_string(p));
        }
}
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
                namespace impl = ekf_implementation;

                x_ = f * x_;
                p_ = f * p_ * f_t + q;

                impl::check_x_p("LKF predict", x_, p_);
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
                namespace impl = ekf_implementation;

                x_ = f(x_);

                const Matrix<N, N, T> fjx = fj(x_);
                p_ = fjx * p_ * fjx.transposed() + q;

                impl::check_x_p("EKF predict", x_, p_);
        }

        template <std::size_t M>
        void update(
                // Measurement function
                const Matrix<M, N, T>& h,
                // Measurement function transposed
                const Matrix<N, M, T>& ht,
                // measurement covariance
                const Matrix<M, M, T>& r,
                // Measurement
                const Vector<M, T>& z,
                // H infinity parameter
                const T theta = 0)
        {
                namespace impl = ekf_implementation;

                const Matrix<N, M, T> k = [&]()
                {
                        if (theta == 0)
                        {
                                const Matrix<N, M, T> p_ht = p_ * ht;
                                return p_ht * (h * p_ht + r).inversed();
                        }
                        const Matrix<N, M, T> ht_ri = ht * r.inversed();
                        return impl::h_infinity_k("LKF update", theta, p_, h, ht_ri);
                }();

                x_ = x_ + k * (z - h * x_);

                const Matrix<N, N, T> i_kh = IDENTITY_MATRIX<N, T> - k * h;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();

                impl::check_x_p("LKF update", x_, p_);
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
                const ResidualZ residual_z,
                // H infinity parameter
                const T theta = 0)
        {
                namespace impl = ekf_implementation;

                const Matrix<M, N, T> hjx = hj(x_);

                const Matrix<N, M, T> k = [&]()
                {
                        if (theta == 0)
                        {
                                const Matrix<N, M, T> p_hjxt = p_ * hjx.transposed();
                                return p_hjxt * (hjx * p_hjxt + r).inversed();
                        }
                        const Matrix<N, M, T> ht_ri = hjx.transposed() * r.inversed();
                        return impl::h_infinity_k("EKF update", theta, p_, hjx, ht_ri);
                }();

                x_ = add_x(x_, k * residual_z(z, h(x_)));

                const Matrix<N, N, T> i_kh = IDENTITY_MATRIX<N, T> - k * hjx;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();

                impl::check_x_p("EKF update", x_, p_);
        }
};
}
