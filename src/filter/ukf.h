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

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <tuple>

namespace ns::filter
{
namespace ukf_implementation
{
template <std::size_t N, typename T, std::size_t POINT_COUNT>
std::tuple<Vector<N, T>, Matrix<N, N, T>> unscented_transform(
        const std::array<Vector<N, T>, POINT_COUNT>& points,
        const Vector<POINT_COUNT, T>& wm,
        const Vector<POINT_COUNT, T>& wc,
        const Matrix<N, N, T>& noise_covariance)
{
        Vector<N, T> x(0);
        for (std::size_t i = 0; i < POINT_COUNT; ++i)
        {
                x += wm[i] * points[i];
        }

        Matrix<N, N, T> p = noise_covariance;
        for (std::size_t i = 0; i < POINT_COUNT; ++i)
        {
                const Vector<N, T> v = points[i] - x;
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

template <std::size_t N, std::size_t M, typename T, std::size_t POINT_COUNT>
Matrix<N, M, T> state_measurement_cross_covariance(
        const Vector<POINT_COUNT, T>& wc,
        const std::array<Vector<N, T>, POINT_COUNT>& sigmas_f,
        const Vector<N, T>& x,
        const std::array<Vector<M, T>, POINT_COUNT>& sigmas_h,
        const Vector<M, T>& z)
{
        Matrix<N, M, T> res(0);
        for (std::size_t i = 0; i < POINT_COUNT; ++i)
        {
                const Vector<N, T> s = sigmas_f[i] - x;
                const Vector<M, T> m = sigmas_h[i] - z;
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
}

template <std::size_t N, typename T, template <std::size_t, typename> typename SigmaPoints>
class Ukf final
{
        static constexpr std::size_t POINT_COUNT =
                std::tuple_size_v<decltype(std::declval<SigmaPoints<N, T>>().points({}, {}))>;

        static_assert(POINT_COUNT >= 2 * N + 1);

        SigmaPoints<N, T> sigma_points_;

        // state mean
        Vector<N, T> x_;

        // state covariance
        Matrix<N, N, T> p_;

        std::array<Vector<N, T>, POINT_COUNT> sigmas_f_;

public:
        Ukf(SigmaPoints<N, T> sigma_points,
            const Vector<N, T>& x /*state mean*/,
            const Matrix<N, N, T>& p /*state covariance*/)
                : sigma_points_(std::move(sigma_points)),
                  x_(x),
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

        template <typename F>
        void predict(const F& f /*state transition function*/, const Matrix<N, N, T>& q /*process covariance*/)
        {
                namespace impl = ukf_implementation;

                const std::array<Vector<N, T>, POINT_COUNT> sigmas = sigma_points_.points(x_, p_);

                for (std::size_t i = 0; i < POINT_COUNT; ++i)
                {
                        sigmas_f_[i] = f(sigmas[i]);
                }

                std::tie(x_, p_) = impl::unscented_transform(sigmas_f_, sigma_points_.wm(), sigma_points_.wc(), q);
        }

        template <std::size_t M, typename H>
        void update(
                const H& h /*measurement function*/,
                const Matrix<M, M, T>& r /* measurement covariance*/,
                const Vector<M, T>& z /*measurement*/)
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

                const auto [x_z, p_z] = impl::unscented_transform(sigmas_h, sigma_points_.wm(), sigma_points_.wc(), r);

                const Matrix<N, M, T> p_xz =
                        impl::state_measurement_cross_covariance(sigma_points_.wc(), sigmas_f_, x_, sigmas_h, x_z);

                const Matrix<N, M, T> k = p_xz * p_z.inversed();

                x_ = x_ + k * (z - x_z);
                p_ = p_ - k * p_z * k.transposed();
        }
};
}
