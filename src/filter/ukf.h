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

10.4 The Unscented Transform
10.5 The Unscented Kalman Filter
10.11 Implementation of the UKF
*/

#pragma once

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <tuple>

namespace ns::filter
{
namespace ukf_implementation
{
template <std::size_t N, typename T, std::size_t POINT_COUNT, typename Mean, typename Residual>
std::tuple<Vector<N, T>, Matrix<N, N, T>> unscented_transform(
        const std::array<Vector<N, T>, POINT_COUNT>& points,
        const Vector<POINT_COUNT, T>& wm,
        const Vector<POINT_COUNT, T>& wc,
        const Matrix<N, N, T>& noise_covariance,
        const Mean mean,
        const Residual residual)
{
        const Vector<N, T> x = mean(points, wm);

        Matrix<N, N, T> p = noise_covariance;
        for (std::size_t i = 0; i < POINT_COUNT; ++i)
        {
                const Vector<N, T> v = residual(points[i], x);
                for (std::size_t r = 0; r < N; ++r)
                {
                        for (std::size_t c = 0; c < N; ++c)
                        {
                                p(r, c) += wc[i] * v[r] * v[c];
                        }
                }
        }

        return {x, p};
}

template <std::size_t N, std::size_t M, typename T, std::size_t POINT_COUNT, typename ResidualX, typename ResidualZ>
Matrix<N, M, T> state_measurement_cross_covariance(
        const Vector<POINT_COUNT, T>& wc,
        const std::array<Vector<N, T>, POINT_COUNT>& sigmas_f,
        const Vector<N, T>& x,
        const std::array<Vector<M, T>, POINT_COUNT>& sigmas_h,
        const Vector<M, T>& z,
        const ResidualX residual_x,
        const ResidualZ residual_z)
{
        Matrix<N, M, T> res(0);
        for (std::size_t i = 0; i < POINT_COUNT; ++i)
        {
                const Vector<N, T> s = residual_x(sigmas_f[i], x);
                const Vector<M, T> m = residual_z(sigmas_h[i], z);
                for (std::size_t r = 0; r < N; ++r)
                {
                        for (std::size_t c = 0; c < M; ++c)
                        {
                                res(r, c) += wc[i] * s[r] * m[c];
                        }
                }
        }
        return res;
}

template <std::size_t N, typename T>
void check_x_p(const char* const name, const Vector<N, T>& x, const Matrix<N, N, T>& p)
{
        for (std::size_t i = 0; i < N; ++i)
        {
                if (!(p(i, i) > 0))
                {
                        error(std::string(name) + ", diagonal is not positive\nx\n" + to_string(x) + "\np\n"
                              + to_string(p));
                }
        }
}
}

template <std::size_t N, typename T, typename SigmaPoints, typename AddX, typename MeanX, typename ResidualX>
class Ukf final
{
        static constexpr std::size_t POINT_COUNT =
                std::tuple_size_v<std::remove_cvref_t<decltype(std::declval<SigmaPoints>().wm())>>;

        static_assert(POINT_COUNT >= 2 * N + 1);

        SigmaPoints sigma_points_;

        // The sum of the two state vectors
        // Vector<N, T> operator()(
        //   const Vector<N, T>& a,
        //   const Vector<N, T>& b) const
        AddX add_x_;
        // The mean of the sigma points and weights
        // Vector<N, T> operator()(
        //   const std::array<Vector<N, T>, COUNT>& p,
        //   const Vector<COUNT, T>& w) const
        MeanX mean_x_;
        // The residual between the two vectors
        // Vector<N, T> operator()(
        //   const Vector<N, T>& a,
        //   const Vector<N, T>& b) const
        ResidualX residual_x_;

        // state mean
        Vector<N, T> x_;

        // state covariance
        Matrix<N, N, T> p_;

        std::array<Vector<N, T>, POINT_COUNT> sigmas_f_;

public:
        Ukf(SigmaPoints sigma_points,
            AddX add_x,
            MeanX mean_x,
            ResidualX residual_x,
            const Vector<N, T>& x,
            const Matrix<N, N, T>& p)
                : sigma_points_(std::move(sigma_points)),
                  add_x_(std::move(add_x)),
                  mean_x_(std::move(mean_x)),
                  residual_x_(std::move(residual_x)),
                  x_(x),
                  p_(p)
        {
                namespace impl = ukf_implementation;

                impl::check_x_p("UKF constructor", x_, p_);
        }

        [[nodiscard]] const Vector<N, T>& x() const
        {
                return x_;
        }

        [[nodiscard]] const Matrix<N, N, T>& p() const
        {
                return p_;
        }

        template <typename F>
        void predict(
                // State transition function
                // Vector<N, T> operator()(
                //   const Vector<N, T>& x) const
                const F f,
                // Process covariance
                const Matrix<N, N, T>& q)
        {
                namespace impl = ukf_implementation;

                const std::array<Vector<N, T>, POINT_COUNT> sigmas = sigma_points_.points(x_, p_);

                for (std::size_t i = 0; i < POINT_COUNT; ++i)
                {
                        sigmas_f_[i] = f(sigmas[i]);
                }

                std::tie(x_, p_) = impl::unscented_transform(
                        sigmas_f_, sigma_points_.wm(), sigma_points_.wc(), q, mean_x_, residual_x_);

                impl::check_x_p("UKF predict", x_, p_);
        }

        template <std::size_t M, typename H, typename MeanZ, typename ResidualZ>
        void update(
                // Measurement function
                // Vector<M, T> operator()(
                //   const Vector<N, T>& x) const
                const H h,
                // Measurement covariance
                const Matrix<M, M, T>& r,
                // Measurement
                const Vector<M, T>& z,
                // The mean of the sigma points and weights
                // Vector<M, T> operator()(
                //   const std::array<Vector<M, T>, COUNT>& p,
                //   const Vector<COUNT, T>& w) const
                const MeanZ mean_z,
                // The residual between the two measurement vectors
                // Vector<M, T> operator()(
                //   const Vector<M, T>& a,
                //   const Vector<M, T>& b) const
                const ResidualZ residual_z)
        {
                namespace impl = ukf_implementation;

                const std::array<Vector<M, T>, POINT_COUNT> sigmas_h = [&]()
                {
                        std::array<Vector<M, T>, POINT_COUNT> res;
                        for (std::size_t i = 0; i < POINT_COUNT; ++i)
                        {
                                res[i] = h(sigmas_f_[i]);
                        }
                        return res;
                }();

                const auto [x_z, p_z] = impl::unscented_transform(
                        sigmas_h, sigma_points_.wm(), sigma_points_.wc(), r, mean_z, residual_z);

                impl::check_x_p("UKF update measurement", x_z, p_z);

                const Matrix<N, M, T> p_xz = impl::state_measurement_cross_covariance(
                        sigma_points_.wc(), sigmas_f_, x_, sigmas_h, x_z, residual_x_, residual_z);

                const Matrix<N, M, T> k = p_xz * p_z.inversed();

                x_ = add_x_(x_, k * residual_z(z, x_z));
                p_ = p_ - p_xz * k.transposed();

                impl::check_x_p("UKF update", x_, p_);
        }
};
}
