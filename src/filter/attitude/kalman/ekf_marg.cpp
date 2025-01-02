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

#include "ekf_marg.h"

#include "constant.h"
#include "cross_matrix.h"
#include "ekf_matrix.h"
#include "ekf_utility.h"
#include "integrator.h"
#include "quaternion.h"
#include "utility.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
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
void EkfMarg<T>::predict(const Vector3& w0, const Vector3& w1, const T variance_r, const T variance_w, const T dt)
{
        ASSERT(q_);

        const Vector3 wb0 = w0 - b_;
        const Vector3 wb1 = w1 - b_;

        q_ = first_order_quaternion_integrator(*q_, wb0, wb1, dt).normalized();

        const Matrix6 phi = ekf_state_transition_matrix_6(wb1, dt);
        const Matrix6 q = ekf_noise_covariance_matrix_6(wb1, variance_r, variance_w, dt);

        p_ = phi * p_ * phi.transposed() + q;
}

template <typename T>
template <std::size_t N>
void EkfMarg<T>::update(const std::array<Update, N>& data)
{
        ASSERT(q_);

        numerical::Vector<3 * N, T> z;
        numerical::Vector<3 * N, T> hx;
        numerical::Matrix<3 * N, 6, T> h(numerical::ZERO_MATRIX);
        numerical::Matrix<3 * N, 3 * N, T> r(numerical::ZERO_MATRIX);

        for (std::size_t i = 0; i < N; ++i)
        {
                const Vector3 hx_i = data[i].prediction;
                const Matrix3 h_i = cross_matrix<1>(hx_i);
                const Vector3 z_i = data[i].measurement;
                const T variance_i = data[i].variance;
                const std::size_t offset = 3 * i;
                numerical::set_block(hx, offset, hx_i);
                numerical::set_block(h, offset, 0, h_i);
                numerical::set_block(z, offset, z_i);
                for (std::size_t j = offset; j < offset + 3; ++j)
                {
                        r[j, j] = variance_i;
                }
        }

        const numerical::Matrix<6, 3 * N, T> ht = h.transposed();
        const numerical::Matrix<3 * N, 3 * N, T> s = h * p_ * ht + r;
        const numerical::Matrix<6, 3 * N, T> k = p_ * ht * s.inversed();
        const Vector6 dx = k * (z - hx);

        const Vector3 dx_q = numerical::block<0, 3>(dx);
        const Vector3 dx_b = numerical::block<3, 3>(dx);

        const Quaternion<T> dq = ekf_delta_quaternion(dx_q / T{2});
        q_ = (dq * *q_).normalized();

        b_ += dx_b;

        const Matrix6 i_kh = numerical::IDENTITY_MATRIX<6, T> - k * h;
        p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();
}

template <typename T>
void EkfMarg<T>::init()
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
void EkfMarg<T>::init_acc(const Vector3& a)
{
        acc_data_ += a;
        ++acc_count_;

        init();
}

template <typename T>
void EkfMarg<T>::init_mag(const Vector3& m)
{
        mag_data_ += m;
        ++mag_count_;

        init();
}

template <typename T>
void EkfMarg<T>::init_acc_mag(const Vector3& a, const Vector3& m)
{
        acc_data_ += a;
        ++acc_count_;
        mag_data_ += m;
        ++mag_count_;

        init();
}

template <typename T>
void EkfMarg<T>::reset_init()
{
        ASSERT(!q_);

        acc_data_ = Vector3(0);
        acc_count_ = 0;
        mag_data_ = Vector3(0);
        mag_count_ = 0;
}

template <typename T>
EkfMarg<T>::EkfMarg()
{
        reset_init();
}

template <typename T>
void EkfMarg<T>::update_gyro(const Vector3& w0, const Vector3& w1, const T variance_r, const T variance_w, const T dt)
{
        if (q_)
        {
                predict(w0, w1, variance_r, variance_w, dt);
        }
}

template <typename T>
bool EkfMarg<T>::update_acc(const Vector3& a, const T variance, const T variance_direction)
{
        if (!q_)
        {
                init_acc(a);
                return q_.has_value();
        }

        const T a_norm = a.norm();
        if (!acc_suitable(a_norm))
        {
                return false;
        }

        const Vector3 y = global_to_local(*q_, {0, 1, 0});
        const Vector3 z = global_to_local(*q_, {0, 0, 1});

        const Vector3 zm = a / a_norm;

        update(std::array{
                Update{
                       .measurement = zm,
                       .prediction = z,
                       .variance = variance,
                       },
                Update{
                       .measurement = y,
                       .prediction = y,
                       .variance = variance_direction,
                       }
        });

        return true;
}

template <typename T>
bool EkfMarg<T>::update_mag(const Vector3& m, const T variance, const T variance_direction)
{
        if (!q_)
        {
                init_mag(m);
                return q_.has_value();
        }

        const T m_norm = m.norm();
        if (!mag_suitable(m_norm))
        {
                return false;
        }

        const Vector3 y = global_to_local(*q_, {0, 1, 0});
        const Vector3 z = global_to_local(*q_, {0, 0, 1});

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
                       .prediction = y,
                       .variance = variance / sin2,
                       },
                Update{
                       .measurement = z,
                       .prediction = z,
                       .variance = variance_direction,
                       }
        });

        return true;
}

template <typename T>
bool EkfMarg<T>::update_acc_mag(const Vector3& a, const Vector3& m, const T a_variance, const T m_variance)
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

        const Vector3 y = global_to_local(*q_, {0, 1, 0});
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
                       .prediction = y,
                       .variance = m_variance / sin2,
                       },
                Update{
                       .measurement = zm,
                       .prediction = z,
                       .variance = a_variance,
                       }
        });

        return true;
}

template class EkfMarg<float>;
template class EkfMarg<double>;
template class EkfMarg<long double>;
}
