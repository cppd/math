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

#include "integrator.h"
#include "matrix.h"
#include "quaternion.h"
#include "utility.h"

#include <src/com/error.h>
#include <src/com/exponent.h>
#include <src/filter/attitude/limit.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>

namespace ns::filter::attitude::ekf
{
template <typename T>
class EkfImu final
{
        using Vector = numerical::Vector<3, T>;
        using Matrix = numerical::Matrix<3, 3, T>;

        static constexpr unsigned ACC_COUNT{10};
        Vector acc_data_{0};
        unsigned acc_count_{0};

        std::optional<Quaternion<T>> q_;
        Matrix p_{numerical::ZERO_MATRIX};

        void predict(const Vector& w0, const Vector& w1, const T variance, const T dt)
        {
                ASSERT(q_);

                q_ = first_order_quaternion_integrator(*q_, w0, w1, dt).normalized();

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
                ASSERT(q_);

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

                const Quaternion<T> dq = delta_quaternion(dx / T{2});
                q_ = (dq * *q_).normalized();

                const Matrix i_kh = numerical::IDENTITY_MATRIX<3, T> - k * h;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();
        }

        void update_init(const Vector& a)
        {
                ASSERT(!q_);

                acc_data_ += a;
                ++acc_count_;

                if (acc_count_ < ACC_COUNT)
                {
                        return;
                }

                const Vector a_avg = acc_data_ / T(acc_count_);
                const T a_avg_norm = a_avg.norm();

                if (!acc_suitable(a_avg_norm))
                {
                        reset_init();
                        return;
                }

                q_ = initial_quaternion(a_avg / a_avg_norm);
        }

        void reset_init()
        {
                ASSERT(!q_);

                acc_data_ = Vector(0);
                acc_count_ = 0;
        }

public:
        EkfImu()
        {
                reset_init();
        }

        void update_gyro(
                const numerical::Vector<3, T>& w0,
                const numerical::Vector<3, T>& w1,
                const T variance,
                const T dt)
        {
                if (q_)
                {
                        predict(w0, w1, variance, dt);
                }
        }

        bool update_acc(const numerical::Vector<3, T>& a)
        {
                if (!q_)
                {
                        update_init(a);
                        return q_.has_value();
                }

                const T a_norm = a.norm();
                if (!acc_suitable(a_norm))
                {
                        return false;
                }

                const Vector zm = a / a_norm;
                const Vector z = global_to_local(*q_, {0, 0, 1});
                const Vector y = global_to_local(*q_, {0, 1, 0});

                update(std::array{
                        Update{
                               .measurement = zm,
                               .prediction = z,
                               .variance = square(0.01),
                               },
                        Update{
                               .measurement = y,
                               .prediction = y,
                               .variance = square(0.01),
                               }
                });

                return true;
        }

        [[nodiscard]] std::optional<numerical::Quaternion<T>> attitude() const
        {
                if (q_)
                {
                        return q_->q();
                }
                return std::nullopt;
        }
};
}
