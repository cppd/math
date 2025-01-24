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

#include "ekf_imu.h"

#include "cross_matrix.h"
#include "ekf_utility.h"
#include "integrator.h"
#include "matrix.h"
#include "quaternion.h"

#include <src/com/error.h>
#include <src/filter/attitude/limit.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T>
void EkfImu<T>::predict(const Vector3& w0, const Vector3& w1, const T variance, const T dt)
{
        ASSERT(q_);

        q_ = first_order_quaternion_integrator(*q_, w0, w1, dt).normalized();

        const Matrix3 phi = ekf_state_transition_matrix_3(w1, dt);
        const Matrix3 q = noise_covariance_matrix_3(variance, dt);

        p_ = phi * p_ * phi.transposed() + q;
}

template <typename T>
template <std::size_t N>
void EkfImu<T>::update(const std::array<Update, N>& data)
{
        ASSERT(q_);

        numerical::Vector<3 * N, T> z;
        numerical::Vector<3 * N, T> hx;
        numerical::Matrix<3 * N, 3, T> h;
        numerical::Vector<3 * N, T> r;

        for (std::size_t i = 0; i < N; ++i)
        {
                const Vector3 hx_i = global_to_local(*q_, data[i].reference);
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

        const numerical::Matrix<3, 3 * N, T> ht = h.transposed();
        const numerical::Matrix<3 * N, 3 * N, T> s = add_diagonal(h * p_ * ht, r);
        const numerical::Matrix<3, 3 * N, T> k = p_ * ht * s.inversed();
        const Vector3 dx = k * (z - hx);

        const Quaternion<T> dq = ekf_delta_quaternion(dx / T{2});
        q_ = (dq * *q_).normalized();

        const Matrix3 i_kh = numerical::IDENTITY_MATRIX<3, T> - k * h;
        p_ = i_kh * p_ * i_kh.transposed() + mul_diagonal(k, r) * k.transposed();
}

template <typename T>
void EkfImu<T>::update_gyro(const Vector3& w0, const Vector3& w1, const T variance, const T dt)
{
        if (q_)
        {
                predict(w0, w1, variance, dt);
        }
}

template <typename T>
bool EkfImu<T>::update_acc(const Vector3& a, const T variance, const T variance_direction)
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

template class EkfImu<float>;
template class EkfImu<double>;
template class EkfImu<long double>;
}
