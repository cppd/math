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
#include "matrices.h"
#include "quaternion.h"
#include "ukf_utility.h"

#include <src/com/error.h>
#include <src/filter/core/sigma_points.h>
#include <src/filter/core/ukf_transform.h>
#include <src/numerical/matrix.h>
#include <src/numerical/rotation.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>

namespace ns::filter::attitude::kalman
{
namespace
{
template <typename T>
constexpr core::SigmaPoints<6, T>::Parameters SIGMA_POINTS_PARAMETERS{
        .alpha = 1,
        .beta = 2,
        .kappa = 1,
};

template <typename T>
numerical::Vector<3, T> to_error(const numerical::Vector<6, T>& v)
{
        return {v[0], v[1], v[2]};
}

template <typename T>
numerical::Vector<3, T> to_bias(const numerical::Vector<6, T>& v)
{
        return {v[3], v[4], v[5]};
}

template <typename T>
numerical::Vector<6, T> to_state(const numerical::Vector<3, T>& error, const numerical::Vector<3, T>& bias)
{
        return {error[0], error[1], error[2], bias[0], bias[1], bias[2]};
}

template <std::size_t COUNT, typename T>
std::array<Quaternion<T>, COUNT> propagate_quaternions(
        const Quaternion<T>& q,
        const std::array<numerical::Vector<6, T>, COUNT>& sigma_points,
        const numerical::Vector<3, T>& w0,
        const numerical::Vector<3, T>& w1,
        const T dt)
{
        std::array<Quaternion<T>, COUNT> res;
        for (std::size_t i = 0; i < COUNT; ++i)
        {
                const numerical::Vector<3, T> error = to_error(sigma_points[i]);
                const numerical::Vector<3, T> bias = to_bias(sigma_points[i]);
                const numerical::Vector<3, T> wb0 = w0 - bias;
                const numerical::Vector<3, T> wb1 = w1 - bias;
                const Quaternion<T> point_quaternion = error_to_quaternion(error, q);
                res[i] = first_order_quaternion_integrator(point_quaternion, wb0, wb1, dt).normalized();
        }
        return res;
}

template <std::size_t COUNT, typename T>
std::array<numerical::Vector<6, T>, COUNT> propagate_points(
        const std::array<numerical::Vector<6, T>, COUNT>& sigma_points,
        const std::array<Quaternion<T>, COUNT>& propagated_quaternions)
{
        ASSERT(to_error(sigma_points[0]).is_zero());

        const Quaternion<T> center_inversed = propagated_quaternions[0].conjugate();

        std::array<numerical::Vector<6, T>, COUNT> res;
        res[0] = sigma_points[0];
        for (std::size_t i = 1; i < COUNT; ++i)
        {
                const numerical::Vector<3, T> error = quaternion_to_error(propagated_quaternions[i], center_inversed);
                const numerical::Vector<3, T> bias = to_bias(sigma_points[i]);
                res[i] = to_state(error, bias);
        }
        return res;
}
}

template <typename T>
void UkfMarg<T>::predict(const Vector3& w0, const Vector3& w1, const T variance_r, const T variance_w, const T dt)
{
        numerical::set_block<0>(x_, Vector3(0));

        const std::array<Vector6, POINT_COUNT> sigma_points = sigma_points_.points(x_, p_);
        ASSERT(sigma_points[0] == x_);

        propagated_quaternions_ = propagate_quaternions(q_, sigma_points, w0, w1, dt);
        propagated_points_ = propagate_points(sigma_points, propagated_quaternions_);

        const Matrix6 q = noise_covariance_matrix_6(w1 - to_bias(x_), variance_r, variance_w, dt);

        std::tie(x_, p_) = core::unscented_transform(propagated_points_, sigma_points_.wm(), sigma_points_.wc(), q);

        q_ = error_to_quaternion(to_error(x_), propagated_quaternions_[0]).normalized();

        predicted_ = true;
}

template <typename T>
template <std::size_t N>
void UkfMarg<T>::update(const std::array<Update, N>& data)
{
        if (!predicted_)
        {
                return;
        }
        predicted_ = false;

        numerical::Vector<3 * N, T> z;
        numerical::Vector<3 * N, T> r;
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t offset = 3 * i;
                numerical::set_block(z, offset, data[i].measurement);
                numerical::set_block(r, offset, Vector3(data[i].variance));
        }

        std::array<numerical::Vector<3 * N, T>, POINT_COUNT> sigmas_h;
        for (std::size_t i = 0; i < POINT_COUNT; ++i)
        {
                const numerical::Matrix<3, 3, T> attitude =
                        numerical::rotation_quaternion_to_matrix(propagated_quaternions_[i]);

                for (std::size_t j = 0; j < N; ++j)
                {
                        const Vector3 h = attitude * data[j].reference_global;
                        const std::size_t offset = 3 * j;
                        numerical::set_block(sigmas_h[i], offset, h);
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

        q_ = error_to_quaternion(to_error(x_), propagated_quaternions_[0]).normalized();
}

template <typename T>
UkfMarg<T>::UkfMarg(const Quaternion<T>& q, const T variance_error, const T variance_bias)
        : sigma_points_(SIGMA_POINTS_PARAMETERS<T>),
          q_(q),
          x_(0),
          p_(numerical::make_diagonal_matrix<6, T>(
                  {variance_error, variance_error, variance_error, variance_bias, variance_bias, variance_bias}))
{
}

template <typename T>
void UkfMarg<T>::update_gyro(const Vector3& w0, const Vector3& w1, const T variance_r, const T variance_w, const T dt)
{
        predict(w0, w1, variance_r, variance_w, dt);
}

template <typename T>
void UkfMarg<T>::update_z_y(const Vector3& z, const Vector3& y, const T z_variance, const T y_variance)
{
        ASSERT(z.is_unit());
        ASSERT(y.is_unit());

        update(std::array{
                Update{
                       .measurement = y.normalized(),
                       .reference_global = {0, 1, 0},
                       .variance = y_variance,
                       },
                Update{
                       .measurement = z.normalized(),
                       .reference_global = {0, 0, 1},
                       .variance = z_variance,
                       }
        });
}

template <typename T>
[[nodiscard]] numerical::Vector<3, T> UkfMarg<T>::z_local() const
{
        const numerical::Matrix<3, 3, T> attitude = numerical::rotation_quaternion_to_matrix(q_);

        return attitude.column(2);
}

template class UkfMarg<float>;
template class UkfMarg<double>;
template class UkfMarg<long double>;
}
