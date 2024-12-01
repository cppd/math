/*
Copyright (C) 2017-2024 Topological Manifold

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

#pragma once

#include "constant.h"
#include "integrator.h"
#include "matrix.h"
#include "quaternion.h"
#include "rotation.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>

namespace ns::filter::attitude::ekf
{
namespace ekf_implementation
{
template <typename T>
Quaternion<T> make_unit_quaternion(const numerical::Vector<3, T>& v)
{
        const T n2 = v.norm_squared();
        if (n2 <= 1)
        {
                return Quaternion<T>(std::sqrt(1 - n2), v);
        }
        return Quaternion<T>(1, v) / std::sqrt(1 + n2);
}
}

template <typename T>
class Ekf final
{
        using Vector = numerical::Vector<3, T>;
        using Matrix = numerical::Matrix<3, 3, T>;

        static constexpr unsigned ACC_COUNT{10};
        Vector acc_data_{0};
        unsigned acc_count_{0};

        Quaternion<T> q_;
        Matrix p_{numerical::ZERO_MATRIX};

        Vector global_to_local(const Vector& global)
        {
                return rotate_vector(q_.q().conjugate(), global);
        }

        void predict(const Vector& w0, const Vector& w1, const T variance, const T dt)
        {
                q_ = first_order_quaternion_integrator(q_, w0, w1, dt).normalized();

                const Matrix phi = state_transition_matrix_3(w1, dt);
                const Matrix q = noise_covariance_matrix_3(variance, dt);

                p_ = phi * p_ * phi.transposed() + q;
        }

        struct Update final
        {
                Vector measurement;
                Vector prediction;
                T variance;
        };

        template <std::size_t N>
        void update(const std::array<Update, N>& data)
        {
                namespace impl = ekf_implementation;

                numerical::Vector<3 * N, T> z;
                numerical::Vector<3 * N, T> hx;
                numerical::Matrix<3 * N, 3, T> h;
                numerical::Matrix<3 * N, 3 * N, T> r(numerical::ZERO_MATRIX);

                for (std::size_t i = 0; i < N; ++i)
                {
                        const Vector hx_i = data[i].prediction;
                        const Matrix h_i = cross_matrix<1>(hx_i);
                        const Vector z_i = data[i].measurement;
                        const T variance_i = data[i].variance;
                        for (std::size_t b = 3 * i, j = 0; j < 3; ++b, ++j)
                        {
                                z[b] = z_i[j];
                                hx[b] = hx_i[j];
                                h.row(b) = h_i.row(j);
                                r[b, b] = variance_i;
                        }
                }

                const numerical::Matrix<3, 3 * N, T> ht = h.transposed();
                const numerical::Matrix<3 * N, 3 * N, T> s = h * p_ * ht + r;
                const numerical::Matrix<3, 3 * N, T> k = p_ * ht * s.inversed();
                const Vector dx = k * (z - hx);

                const Quaternion<T> q = impl::make_unit_quaternion(dx / T{2});
                q_ = (q * q_).normalized();

                const Matrix i_kh = numerical::IDENTITY_MATRIX<3, T> - k * h;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();
        }

        [[nodiscard]] bool has_attitude() const
        {
                return acc_count_ >= ACC_COUNT;
        }

        void update_init(const Vector& a_norm)
        {
                ASSERT(acc_count_ < ACC_COUNT);
                acc_data_ += a_norm;
                ++acc_count_;
                if (acc_count_ >= ACC_COUNT)
                {
                        q_ = Quaternion(initial_quaternion(acc_data_ / T(acc_count_)));
                }
        }

public:
        void update_gyro(
                const numerical::Vector<3, T>& w0,
                const numerical::Vector<3, T>& w1,
                const T variance,
                const T dt)
        {
                if (has_attitude())
                {
                        predict(w0, w1, variance, dt);
                }
        }

        void update_acc(const numerical::Vector<3, T>& a)
        {
                const T a_norm = a.norm();
                if (!(a_norm >= MIN_ACCELERATION<T> && a_norm <= MAX_ACCELERATION<T>))
                {
                        return;
                }

                if (!has_attitude())
                {
                        update_init(a / a_norm);
                        return;
                }

                update(std::array{
                        Update{
                               .measurement = a / a_norm,
                               .prediction = global_to_local({0, 0, 1}),
                               .variance = square(0.01),
                               },
                        Update{
                               .measurement = global_to_local({0, 1, 0}),
                               .prediction = global_to_local({0, 1, 0}),
                               .variance = square(0.01),
                               }
                });
        }

        [[nodiscard]] std::optional<numerical::Quaternion<T>> attitude() const
        {
                if (has_attitude())
                {
                        return q_.q();
                }
                return std::nullopt;
        }
};

template <typename T>
class EkfB final
{
        using Vector3 = numerical::Vector<3, T>;
        using Vector6 = numerical::Vector<6, T>;
        using Matrix3 = numerical::Matrix<3, 3, T>;
        using Matrix6 = numerical::Matrix<6, 6, T>;

        static constexpr unsigned INIT_COUNT{10};
        Vector3 acc_data_{0};
        unsigned acc_count_{0};
        Vector3 mag_data_{0};
        unsigned mag_count_{0};
        bool has_attitude_{false};

        Quaternion<T> q_;
        Vector3 b_{0};
        Matrix6 p_{numerical::ZERO_MATRIX};

        Vector3 global_to_local(const Vector3& global)
        {
                return rotate_vector(q_.q().conjugate(), global);
        }

        void predict(const Vector3& w0, const Vector3& w1, const T variance_r, const T variance_w, const T dt)
        {
                const Vector3 wb0 = w0 - b_;
                const Vector3 wb1 = w1 - b_;

                q_ = first_order_quaternion_integrator(q_, wb0, wb1, dt).normalized();

                const Matrix6 phi = state_transition_matrix_6(wb1, dt);
                const Matrix6 q = noise_covariance_matrix_6(wb1, variance_r, variance_w, dt);

                p_ = phi * p_ * phi.transposed() + q;
        }

        struct Update final
        {
                Vector3 measurement;
                Vector3 prediction;
                T variance;
        };

        template <std::size_t N>
        void update(const std::array<Update, N>& data)
        {
                namespace impl = ekf_implementation;

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
                        for (std::size_t b = 3 * i, j = 0; j < 3; ++b, ++j)
                        {
                                z[b] = z_i[j];
                                hx[b] = hx_i[j];
                                for (std::size_t k = 0; k < 3; ++k)
                                {
                                        h.row(b)[k] = h_i.row(j)[k];
                                }
                                r[b, b] = variance_i;
                        }
                }

                const numerical::Matrix<6, 3 * N, T> ht = h.transposed();
                const numerical::Matrix<3 * N, 3 * N, T> s = h * p_ * ht + r;
                const numerical::Matrix<6, 3 * N, T> k = p_ * ht * s.inversed();

                const Vector6 dx = k * (z - hx);
                const Vector3 dxq = dx.template segment<0, 3>();
                const Vector3 dxb = dx.template segment<3, 3>();

                const Quaternion<T> q = impl::make_unit_quaternion(dxq / T{2});
                q_ = (q * q_).normalized();

                b_ += dxb;

                const Matrix6 i_kh = numerical::IDENTITY_MATRIX<6, T> - k * h;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();
        }

        void init()
        {
                ASSERT(!has_attitude_);
                if (acc_count_ < INIT_COUNT || mag_count_ < INIT_COUNT)
                {
                        return;
                }
                const Vector3 a = acc_data_ / T(acc_count_);
                const Vector3 m = mag_data_ / T(mag_count_);
                q_ = Quaternion(initial_quaternion(a, m));
                has_attitude_ = true;
        }

        void update_init_acc(const Vector3& a_norm)
        {
                acc_data_ += a_norm;
                ++acc_count_;
                init();
        }

        void update_init_mag(const Vector3& m_norm)
        {
                mag_data_ += m_norm;
                ++mag_count_;
                init();
        }

public:
        void update_gyro(
                const numerical::Vector<3, T>& w0,
                const numerical::Vector<3, T>& w1,
                const T variance_r,
                const T variance_w,
                const T dt)
        {
                if (has_attitude_)
                {
                        predict(w0, w1, variance_r, variance_w, dt);
                }
        }

        void update_acc(const numerical::Vector<3, T>& a)
        {
                const T a_norm = a.norm();
                if (!(a_norm >= MIN_ACCELERATION<T> && a_norm <= MAX_ACCELERATION<T>))
                {
                        return;
                }

                if (!has_attitude_)
                {
                        update_init_acc(a / a_norm);
                        return;
                }

                update(std::array{
                        Update{
                               .measurement = a / a_norm,
                               .prediction = global_to_local({0, 0, 1}),
                               .variance = square(0.01),
                               },
                        Update{
                               .measurement = global_to_local({0, 1, 0}),
                               .prediction = global_to_local({0, 1, 0}),
                               .variance = square(0.01),
                               }
                });
        }

        void update_mag(const numerical::Vector<3, T>& m)
        {
                const T m_norm = m.norm();
                if (!(m_norm >= MIN_MAGNETIC_FIELD<T> && m_norm <= MAX_MAGNETIC_FIELD<T>))
                {
                        return;
                }

                if (!has_attitude_)
                {
                        update_init_mag(m / m_norm);
                        return;
                }

                const Vector3 z = global_to_local({0, 0, 1});
                const Vector3 x = cross(m / m_norm, z);

                if (!(x.norm_squared() > square(T{0.01})))
                {
                        return;
                }

                const Vector3 y = cross(z, x);

                update(std::array{
                        Update{
                               .measurement = y.normalized(),
                               .prediction = global_to_local({0, 1, 0}),
                               .variance = square(0.01),
                               }
                });
        }

        [[nodiscard]] std::optional<numerical::Quaternion<T>> attitude() const
        {
                if (has_attitude_)
                {
                        return q_.q();
                }
                return std::nullopt;
        }

        [[nodiscard]] numerical::Vector<3, T> bias() const
        {
                return b_;
        }
};
}
