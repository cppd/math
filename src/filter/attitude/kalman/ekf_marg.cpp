/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "ekf_marg.h"

#include "cross_matrix.h"
#include "ekf_utility.h"
#include "integrator.h"
#include "matrices.h"
#include "quaternion.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/rotation.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T>
void EkfMarg<T>::predict(const Vector3& w0, const Vector3& w1, const T variance_r, const T variance_w, const T dt)
{
        const Vector3 wb0 = w0 - b_;
        const Vector3 wb1 = w1 - b_;

        q_ = first_order_quaternion_integrator(q_, wb0, wb1, dt).normalized();

        const Matrix6 phi = ekf_state_transition_matrix_6(wb1, dt);
        const Matrix6 q = noise_covariance_matrix_6(wb1, variance_r, variance_w, dt);

        p_ = phi * p_ * phi.transposed() + q;
}

template <typename T>
template <std::size_t N>
void EkfMarg<T>::update(const std::array<Update, N>& data)
{
        numerical::Vector<3 * N, T> z;
        numerical::Vector<3 * N, T> hx;
        numerical::Matrix<3 * N, 6, T> h(numerical::ZERO_MATRIX);
        numerical::Vector<3 * N, T> r;

        for (std::size_t i = 0; i < N; ++i)
        {
                const Vector3 hx_i = data[i].reference_local;
                const Matrix3 h_i = cross_matrix<1>(hx_i);
                const auto& m_i = data[i].measurement;
                const Vector3 z_i = m_i ? *m_i : hx_i;
                const Vector3 r_i(data[i].variance);
                const std::size_t offset = 3 * i;
                numerical::set_block(hx, offset, hx_i);
                numerical::set_block(h, offset, 0, h_i);
                numerical::set_block(z, offset, z_i);
                numerical::set_block(r, offset, r_i);
        }

        const numerical::Matrix<6, 3 * N, T> ht = h.transposed();
        const numerical::Matrix<3 * N, 3 * N, T> s = add_diagonal(h * p_ * ht, r);
        const numerical::Matrix<6, 3 * N, T> k = p_ * ht * s.inversed();
        const Vector6 dx = k * (z - hx);

        const Vector3 dx_q = numerical::block<0, 3>(dx);
        const Vector3 dx_b = numerical::block<3, 3>(dx);

        const Quaternion<T> dq = ekf_delta_quaternion(dx_q / T{2});
        q_ = (dq * q_).normalized();

        b_ += dx_b;

        const Matrix6 i_kh = numerical::IDENTITY_MATRIX<6, T> - k * h;
        p_ = i_kh * p_ * i_kh.transposed() + mul_diagonal(k, r) * k.transposed();
}

template <typename T>
EkfMarg<T>::EkfMarg(const Quaternion<T>& q, const T variance_error, const T variance_bias)
        : q_(q),
          b_(0),
          p_(numerical::make_diagonal_matrix<6, T>(
                  {variance_error, variance_error, variance_error, variance_bias, variance_bias, variance_bias}))
{
}

template <typename T>
void EkfMarg<T>::update_gyro(const Vector3& w0, const Vector3& w1, const T variance_r, const T variance_w, const T dt)
{
        predict(w0, w1, variance_r, variance_w, dt);
}

template <typename T>
void EkfMarg<T>::update_z(const Vector3& z, const T z_variance, const T y_variance)
{
        ASSERT(z.is_unit());

        const Matrix3 attitude = numerical::rotation_quaternion_to_matrix(q_);

        update(std::array{
                Update{
                       .measurement = z,
                       .reference_local = attitude.column(2),
                       .variance = z_variance,
                       },
                Update{
                       .measurement = std::nullopt,
                       .reference_local = attitude.column(1),
                       .variance = y_variance,
                       }
        });
}

template <typename T>
void EkfMarg<T>::update_y(const Vector3& y, const T y_variance, const T z_variance)
{
        ASSERT(y.is_unit());

        const Matrix3 attitude = numerical::rotation_quaternion_to_matrix(q_);

        update(std::array{
                Update{
                       .measurement = y,
                       .reference_local = attitude.column(1),
                       .variance = y_variance,
                       },
                Update{
                       .measurement = std::nullopt,
                       .reference_local = attitude.column(2),
                       .variance = z_variance,
                       }
        });
}

template <typename T>
void EkfMarg<T>::update_z_y(const Vector3& z, const Vector3& y, const T z_variance, const T y_variance)
{
        ASSERT(z.is_unit());
        ASSERT(y.is_unit());

        const Matrix3 attitude = numerical::rotation_quaternion_to_matrix(q_);

        update(std::array{
                Update{
                       .measurement = y,
                       .reference_local = attitude.column(1),
                       .variance = y_variance,
                       },
                Update{
                       .measurement = z,
                       .reference_local = attitude.column(2),
                       .variance = z_variance,
                       }
        });
}

template <typename T>
[[nodiscard]] EkfMarg<T>::Vector3 EkfMarg<T>::z_local() const
{
        const Matrix3 attitude = numerical::rotation_quaternion_to_matrix(q_);

        return attitude.column(2);
}

template class EkfMarg<float>;
template class EkfMarg<double>;
template class EkfMarg<long double>;
}
