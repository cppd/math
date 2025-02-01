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

#include "ukf_imu.h"

#include "integrator.h"
#include "matrix.h"
#include "ukf_utility.h"

#include <src/com/error.h>
#include <src/filter/attitude/limit.h>
#include <src/filter/core/sigma_points.h>
#include <src/filter/core/ukf_transform.h>
#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <optional>

namespace ns::filter::attitude::kalman
{
namespace
{
template <typename T>
constexpr core::SigmaPoints<3, T>::Parameters SIGMA_POINTS_PARAMETERS{
        .alpha = 1,
        .beta = 2,
        .kappa = 1,
};

template <std::size_t COUNT, typename T>
std::array<Quaternion<T>, COUNT> propagate_quaternions(
        const Quaternion<T>& q,
        const std::array<numerical::Vector<3, T>, COUNT>& sigma_points,
        const numerical::Vector<3, T>& w0,
        const numerical::Vector<3, T>& w1,
        const T dt)
{
        std::array<Quaternion<T>, COUNT> res;
        for (std::size_t i = 0; i < COUNT; ++i)
        {
                const Quaternion<T> point_quaternion = error_to_quaternion(sigma_points[i], q);
                res[i] = first_order_quaternion_integrator(point_quaternion, w0, w1, dt).normalized();
        }
        return res;
}

template <std::size_t COUNT, typename T>
std::array<numerical::Vector<3, T>, COUNT> propagate_points(
        const std::array<numerical::Vector<3, T>, COUNT>& sigma_points,
        const std::array<Quaternion<T>, COUNT>& propagated_quaternions)
{
        ASSERT(sigma_points[0].is_zero());

        const Quaternion<T> center_inversed = propagated_quaternions[0].conjugate();

        std::array<numerical::Vector<3, T>, COUNT> res;
        res[0] = sigma_points[0];
        for (std::size_t i = 1; i < COUNT; ++i)
        {
                res[i] = quaternion_to_error(propagated_quaternions[i], center_inversed);
        }
        return res;
}
}

template <typename T>
void UkfImu<T>::predict(const Vector3& w0, const Vector3& w1, const T variance, const T dt)
{
        ASSERT(q_);

        x_ = Vector3(0);

        const std::array<Vector3, POINT_COUNT> sigma_points = sigma_points_.points(x_, p_);
        ASSERT(sigma_points[0] == x_);

        propagated_quaternions_ = propagate_quaternions(*q_, sigma_points, w0, w1, dt);
        propagated_points_ = propagate_points(sigma_points, propagated_quaternions_);

        const Matrix3 q = noise_covariance_matrix_3(variance, dt);

        std::tie(x_, p_) = core::unscented_transform(propagated_points_, sigma_points_.wm(), sigma_points_.wc(), q);

        q_ = error_to_quaternion(x_, propagated_quaternions_[0]).normalized();

        predicted_ = true;
}

template <typename T>
template <std::size_t N>
void UkfImu<T>::update(const std::array<Update, N>& data)
{
        if (!predicted_)
        {
                return;
        }
        predicted_ = false;

        numerical::Vector<3 * N, T> r;
        for (std::size_t i = 0; i < N; ++i)
        {
                const std::size_t offset = 3 * i;
                numerical::set_block(r, offset, Vector3(data[i].variance));
        }

        std::array<numerical::Vector<3 * N, T>, POINT_COUNT> sigmas_h;
        for (std::size_t i = 0; i < POINT_COUNT; ++i)
        {
                for (std::size_t j = 0; j < N; ++j)
                {
                        const Vector3 h = global_to_local(propagated_quaternions_[i], data[j].reference);
                        const std::size_t offset = 3 * j;
                        numerical::set_block(sigmas_h[i], offset, h);
                }
        }

        const auto [x_z, p_z] = core::unscented_transform(sigmas_h, sigma_points_.wm(), sigma_points_.wc(), r);

        const numerical::Matrix<3, 3 * N, T> p_xz =
                core::cross_covariance(sigma_points_.wc(), propagated_points_, x_, sigmas_h, x_z);

        const numerical::Matrix<3 * N, 3 * N, T> p_z_inversed = p_z.inversed();

        numerical::Vector<3 * N, T> residual;
        for (std::size_t i = 0; i < N; ++i)
        {
                const auto& m = data[i].measurement;
                const std::size_t offset = 3 * i;
                numerical::set_block(residual, offset, m ? (*m - numerical::block<3>(x_z, offset)) : Vector3(0));
        }

        const numerical::Matrix<3, 3 * N, T> k = p_xz * p_z_inversed;

        x_ = x_ + k * residual;
        p_ = p_ - p_xz * k.transposed();

        q_ = error_to_quaternion(x_, propagated_quaternions_[0]).normalized();
}

template <typename T>
UkfImu<T>::UkfImu(const T variance)
        : sigma_points_(SIGMA_POINTS_PARAMETERS<T>),
          x_(0),
          p_(numerical::make_diagonal_matrix<3, T>({variance, variance, variance}))
{
}

template <typename T>
void UkfImu<T>::update_gyro(const Vector3& w0, const Vector3& w1, const T variance, const T dt)
{
        if (q_)
        {
                predict(w0, w1, variance, dt);
        }
}

template <typename T>
bool UkfImu<T>::update_acc(const Vector3& a, const T variance, const T variance_direction)
{
        if (!q_)
        {
                q_ = init_.update(a);
                return q_.has_value();
        }

        const T a_norm = a.norm();
        if (!acc_suitable(a_norm))
        {
                return false;
        }

        update(std::array{
                Update{
                       .measurement = a / a_norm,
                       .reference = {0, 0, 1},
                       .variance = variance,
                       },
                Update{
                       .measurement = std::nullopt,
                       .reference = {0, 1, 0},
                       .variance = variance_direction,
                       }
        });

        return true;
}

template class UkfImu<float>;
template class UkfImu<double>;
template class UkfImu<long double>;
}
