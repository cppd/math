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
#include "rotation.h"

#include <src/com/error.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <cmath>
#include <optional>

namespace ns::filter::attitude
{
namespace fusion_implementation
{
template <typename T>
inline constexpr T MIN_ACC = 9.3; // m/s/s

template <typename T>
inline constexpr T MAX_ACC = 10.3; // m/s/s

template <typename T>
numerical::Matrix<3, 3, T> phi_matrix(const numerical::Vector<3, T>& w, const T dt)
{
        const T n2 = w.norm_squared();
        const T n = std::sqrt(n2);

        const numerical::Matrix<3, 3, T> k0 = (std::sin(n * dt) / n) * cross_matrix<1>(w);
        const numerical::Matrix<3, 3, T> k1 = (1 - std::cos(n * dt) / n2) * cross_matrix<2>(w);

        return numerical::IDENTITY_MATRIX<3, T> - k0 + k1;
}

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
class Fusion final
{
        using Vector = numerical::Vector<3, T>;
        using Matrix = numerical::Matrix<3, 3, T>;

        static constexpr unsigned ACC_COUNT{10};
        numerical::Vector<3, T> acc_data_{0};
        unsigned acc_count_{0};

        Quaternion<T> x_;
        Matrix p_{0};
        Vector w_last_;

        void predict(const numerical::Vector<3, T>& w, const T variance, const T dt)
        {
                namespace impl = fusion_implementation;

                x_ = first_order_quaternion_integrator(x_, w_last_, w, dt).normalized();
                w_last_ = w;

                const Matrix phi = impl::phi_matrix(w, dt);
                const Matrix q(variance * dt);

                p_ = phi * p_ * phi.transposed() + q;
        }

        void update(const numerical::Vector<3, T>& z, const numerical::Vector<3, T>& global, const T variance)
        {
                namespace impl = fusion_implementation;

                const Vector hx = rotate_vector(x_, global);
                const Matrix h = cross_matrix<1>(hx);

                const Matrix ht = h.transposed();
                const Matrix r(variance);
                const Matrix s = h * p_ * ht + r;
                const Matrix k = p_ * ht * s.inversed();
                const Vector dx = k * (z - hx);

                const Quaternion<T> q = impl::make_unit_quaternion(dx / T{2});
                x_ = (q * x_).normalized();

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
                        const numerical::Quaternion<T> q = initial_quaternion(acc_data_ / T(acc_count_));
                        x_ = {q.w(), q.vec()};
                }
        }

public:
        void update_gyro(const numerical::Vector<3, T>& w, const T variance, const T dt)
        {
                predict(w, variance, dt);
        }

        void update_acc(const numerical::Vector<3, T>& a)
        {
                namespace impl = fusion_implementation;

                const T n = a.norm();
                if (!(n >= impl::MIN_ACC<T> && n <= impl::MAX_ACC<T>))
                {
                        return;
                }

                const Vector an = a / n;
                if (!has_attitude())
                {
                        update_init(an);
                        return;
                }

                const T sigma = 0.01;
                update(an, {0, 0, 1}, sigma * sigma);
        }

        [[nodiscard]] std::optional<numerical::Quaternion<T>> attitude() const
        {
                if (has_attitude())
                {
                        return {
                                {x_.w(), x_.vec()}
                        };
                }
                return std::nullopt;
        }
};
}
