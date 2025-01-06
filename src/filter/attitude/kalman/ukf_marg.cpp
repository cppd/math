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

#include "constant.h"
#include "integrator.h"
#include "matrix.h"
#include "ukf_utility.h"
#include "utility.h"

#include <src/com/error.h>
#include <src/filter/attitude/limit.h>
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
        const numerical::Vector<3, T>& w,
        const T dt)
{
        std::array<Quaternion<T>, COUNT> res;
        for (std::size_t i = 0; i < COUNT; ++i)
        {
                const numerical::Vector<3, T> error = to_error(sigma_points[i]);
                const numerical::Vector<3, T> bias = to_bias(sigma_points[i]);

                const Quaternion<T> error_quaternion = error_to_quaternion(error, A<T>, F<T>);
                ASSERT(error_quaternion.is_unit());

                const Quaternion<T> point_quaternion = error_quaternion * q;
                ASSERT(point_quaternion.is_unit());

                res[i] = zeroth_order_quaternion_integrator(point_quaternion, w - bias, dt);
                ASSERT(res[i].is_unit());
        }
        return res;
}

template <std::size_t COUNT, typename T>
std::array<numerical::Vector<6, T>, COUNT> propagate_points(
        const std::array<numerical::Vector<6, T>, COUNT>& sigma_points,
        const std::array<Quaternion<T>, COUNT>& propagated_quaternions)
{
        ASSERT(to_error(sigma_points[0]).is_zero());

        const Quaternion<T> zero_inversed = propagated_quaternions[0].conjugate();

        std::array<numerical::Vector<6, T>, COUNT> res;
        res[0] = sigma_points[0];
        for (std::size_t i = 1; i < COUNT; ++i)
        {
                const Quaternion<T> error_quaternion = propagated_quaternions[i] * zero_inversed;
                ASSERT(error_quaternion.is_unit());

                const numerical::Vector<3, T> error = quaternion_to_error(error_quaternion, A<T>, F<T>);
                const numerical::Vector<3, T> bias = to_bias(sigma_points[i]);
                res[i] = to_state(error, bias);
        }
        return res;
}

template <typename T>
Quaternion<T> make_quaternion(const numerical::Vector<6, T>& x, const Quaternion<T>& propagated_quaternion)
{
        const Quaternion dq = error_to_quaternion(to_error(x), A<T>, F<T>);
        ASSERT(dq.is_unit());

        return dq * propagated_quaternion;
}
}

template <typename T>
void UkfMarg<T>::predict(const Vector3& w, T variance_r, T variance_w, const T dt)
{
        ASSERT(q_);

        numerical::set_block<0>(x_, Vector3(0));

        const std::array<Vector6, POINT_COUNT> sigma_points = sigma_points_.points(x_, p_);
        ASSERT(sigma_points[0] == x_);

        propagated_quaternions_ = propagate_quaternions(*q_, sigma_points, w, dt);
        propagated_points_ = propagate_points(sigma_points, propagated_quaternions_);

        const Matrix6 q = noise_covariance_matrix_6(w - to_bias(propagated_points_[0]), variance_r, variance_w, dt);

        std::tie(x_, p_) = core::unscented_transform(propagated_points_, sigma_points_.wm(), sigma_points_.wc(), q);

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
                                sigmas_h[i], offset, global_to_local(propagated_quaternions_[i], data[j].reference));
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

        q_ = make_quaternion(x_, propagated_quaternions_[0]);
}

template <typename T>
void UkfMarg<T>::init()
{
        ASSERT(!q_);

        if (acc_count_ < INIT_COUNT || mag_count_ < INIT_COUNT)
        {
                return;
        }

        const Vector3 a_avg = acc_data_ / T(acc_count_);
        const T a_avg_norm = a_avg.norm();

        if (!acc_suitable(a_avg_norm))
        {
                reset_init();
                return;
        }

        const Vector3 m_avg = mag_data_ / T(mag_count_);
        const T m_avg_norm = m_avg.norm();

        if (!mag_suitable(m_avg_norm))
        {
                reset_init();
                return;
        }

        q_ = initial_quaternion(a_avg / a_avg_norm, m_avg / m_avg_norm);
}

template <typename T>
void UkfMarg<T>::init_acc_mag(const Vector3& a, const Vector3& m)
{
        acc_data_ += a;
        ++acc_count_;
        mag_data_ += m;
        ++mag_count_;

        init();
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
UkfMarg<T>::UkfMarg(const T variance_r, const T variance_w)
        : sigma_points_({
                  .alpha = ALPHA<T>,
                  .beta = BETA<T>,
                  .kappa = KAPPA<T>,
          }),
          x_(0),
          p_(numerical::make_diagonal_matrix<6, T>(
                  {variance_r, variance_r, variance_r, variance_w, variance_w, variance_w}))
{
        reset_init();
}

template <typename T>
void UkfMarg<T>::update_gyro(const Vector3& w, const T variance_r, const T variance_w, const T dt)
{
        if (q_)
        {
                predict(w, variance_r, variance_w, dt);
        }
}

template <typename T>
bool UkfMarg<T>::update_acc_mag(const Vector3& a, const Vector3& m, const T a_variance, const T m_variance)
{
        if (!q_)
        {
                init_acc_mag(a, m);
                return q_.has_value();
        }

        const T a_norm = a.norm();
        if (!acc_suitable(a_norm))
        {
                return false;
        }

        const T m_norm = m.norm();
        if (!mag_suitable(m_norm))
        {
                return false;
        }

        const Vector3 z = global_to_local(*q_, {0, 0, 1});

        const Vector3 zm = a / a_norm;

        const Vector3 x_mag = cross(m / m_norm, z);
        const T sin2 = x_mag.norm_squared();
        if (!(sin2 > square(MIN_SIN_Z_MAG<T>)))
        {
                return false;
        }
        const Vector3 ym = cross(z, x_mag).normalized();

        update(std::array{
                Update{
                       .measurement = ym,
                       .reference = {0, 1, 0},
                       .variance = m_variance / sin2,
                       },
                Update{
                       .measurement = zm,
                       .reference = {0, 0, 1},
                       .variance = a_variance,
                       }
        });

        return true;
}

template class UkfMarg<float>;
template class UkfMarg<double>;
template class UkfMarg<long double>;
}
