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

#include "ukf_marg.h"

#include "integrator.h"
#include "matrix.h"
#include "ukf_utility.h"
#include "utility.h"

#include <src/com/error.h>
#include <src/filter/core/ukf_transform.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>

namespace ns::filter::attitude::kalman
{
namespace
{
template <typename T>
constexpr T A = 0.1;

template <typename T>
constexpr T F = 2 * (A<T> + 1);

template <typename T>
constexpr T ALPHA = 1;

template <typename T>
constexpr T BETA = 0;

template <typename T>
constexpr T KAPPA = 1;
}

template <typename T>
void UkfMarg<T>::predict(const Vector3& w, T variance_r, T variance_w, const T dt)
{
        ASSERT(q_);
        ASSERT((numerical::block<0, 3>(x_) == Vector3(0)));

        propagated_points_ = sigma_points_.points(x_, p_);

        ASSERT((numerical::block<0, 3>(propagated_points_[0]) == Vector3(0)));
        ASSERT((numerical::block<3, 3>(propagated_points_[0]) == numerical::block<3, 3>(x_)));

        for (std::size_t i = 0; i < POINT_COUNT; ++i)
        {
                const Vector3 pv = numerical::block<0, 3>(propagated_points_[i]);
                const Vector3 pw = numerical::block<3, 3>(propagated_points_[i]);

                const Quaternion<T> error_quaternion = error_to_quaternion(pv, A<T>, F<T>);
                ASSERT(error_quaternion.is_unit());

                const Quaternion<T> point_quaternion = error_quaternion * (*q_);
                ASSERT(point_quaternion.is_unit());

                propagated_point_quaternions_[i] = zeroth_order_quaternion_integrator(point_quaternion, w - pw, dt);
                ASSERT(propagated_point_quaternions_[i].is_unit());
        }

        const Quaternion<T> zero_inversed = propagated_point_quaternions_[0].conjugate();

        for (std::size_t i = 1; i < POINT_COUNT; ++i)
        {
                const Quaternion<T> propagated_error_quaternion = propagated_point_quaternions_[i] * zero_inversed;
                ASSERT(propagated_error_quaternion.is_unit());

                const Vector3 propagated_point = quaternion_to_error(propagated_error_quaternion, A<T>, F<T>);
                numerical::set_block<0>(propagated_points_[i], propagated_point);
        }

        const Vector3 b = numerical::block<3, 3>(propagated_points_[0]);
        const Matrix6 q = noise_covariance_matrix_6(w - b, variance_r, variance_w, dt);

        std::tie(x_, p_) = core::unscented_transform(propagated_points_, sigma_points_.wm(), sigma_points_.wc(), q);
}

template <typename T>
template <std::size_t N>
void UkfMarg<T>::update(const std::array<Update, N>& data)
{
        ASSERT(q_);

        numerical::Vector<3 * N, T> z;
        numerical::Matrix<3 * N, 3 * N, T> r(numerical::ZERO_MATRIX);
        for (std::size_t i = 0; i < N; ++i)
        {
                const Vector3 z_i = data[i].measurement;
                const T variance_i = data[i].variance;
                const std::size_t offset = 3 * i;
                numerical::set_block(z, offset, z_i);
                for (std::size_t j = offset; j < offset + 3; ++j)
                {
                        r[j, j] = variance_i;
                }
        }

        std::array<numerical::Vector<3 * N, T>, POINT_COUNT> sigmas_h;
        for (std::size_t i = 0; i < POINT_COUNT; ++i)
        {
                for (std::size_t j = 0; j < N; ++j)
                {
                        const std::size_t offset = 3 * j;
                        numerical::set_block(
                                sigmas_h[i], offset,
                                global_to_local(propagated_point_quaternions_[i], data[j].reference));
                }
        }

        const auto [x_z, p_z] = core::unscented_transform(sigmas_h, sigma_points_.wm(), sigma_points_.wc(), r);

        const numerical::Matrix<6, 3 * N, T> p_xz =
                core::cross_covariance(sigma_points_.wc(), propagated_points_, x_, sigmas_h, x_z);

        const numerical::Matrix<3 * N, 3 * N, T> p_z_inversed = p_z.inversed();
        const numerical::Vector<3 * N, T> residual = z - x_z;
        const numerical::Matrix<6, 3 * N, T> k = p_xz * p_z_inversed;

        x_ = x_ + k * residual;
        p_ = p_ - p_xz * k.transposed();

        const Quaternion dq = error_to_quaternion(numerical::block<0, 3>(x_), A<T>, F<T>);
        ASSERT(dq.is_unit());

        *q_ = dq * propagated_point_quaternions_[0];

        numerical::set_block<0>(x_, Vector3(0));
}

template <typename T>
void UkfMarg<T>::reset_init()
{
        ASSERT(!q_);

        acc_data_ = Vector3(0);
        acc_count_ = 0;
        mag_data_ = Vector3(0);
        mag_count_ = 0;
}

template <typename T>
UkfMarg<T>::UkfMarg()
        : sigma_points_({
                  .alpha = ALPHA<T>,
                  .beta = BETA<T>,
                  .kappa = KAPPA<T>,
          })
{
        reset_init();
}

template class UkfMarg<float>;
template class UkfMarg<double>;
template class UkfMarg<long double>;
}
