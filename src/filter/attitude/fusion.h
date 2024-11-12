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

#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <cmath>

namespace ns::filter::attitude
{
namespace fusion_implementation
{
template <typename T>
numerical::Matrix<3, 3, T> phi_matrix(const numerical::Vector<3, T>& w, const T dt)
{
        const T n2 = w.norm_squared();
        const T n = std::sqrt(n2);

        const auto i = numerical::IDENTITY_MATRIX<3, T>;
        const auto k0 = (std::sin(n * dt) / n) * cross_matrix<1>(w);
        const auto k1 = (1 - std::cos(n * dt) / n2) * cross_matrix<2>(w);

        return i - k0 + k1;
}

template <typename T>
Quaternion<T> make_unit_quaternion(const numerical::Vector<3, T>& v)
{
        const auto n2 = v.norm_squared();
        if (n2 <= 1)
        {
                return Quaternion<T>(v, std::sqrt(1 - n2));
        }
        return Quaternion<T>(v, 1) / std::sqrt(1 + n2);
}
}

template <typename T>
class Fusion final
{
        Quaternion<T> x_;
        numerical::Matrix<3, 3, T> p_;
        numerical::Vector<3, T> w_last_;

        void predict(const numerical::Vector<3, T>& w, const T variance, const T dt)
        {
                namespace impl = fusion_implementation;

                x_ = first_order_quaternion_integrator(x_, w_last_, w, dt).normalized();
                w_last_ = w;

                const auto phi = impl::phi_matrix(w, dt);
                const auto q = numerical::Matrix<3, 3, T>(variance * dt);

                p_ = phi * p_ * phi.transposed() + q;
        }

        void update(const numerical::Vector<3, T>& z, const numerical::Vector<3, T>& global, const T variance)
        {
                namespace impl = fusion_implementation;

                const auto hx = rotate_vector(x_, global);
                const auto h = cross_matrix<1>(hx);

                const auto ht = h.transposed();
                const auto r = numerical::Matrix<3, 3, T>(variance);
                const auto s = h * p_ * ht + r;
                const auto k = p_ * ht * s.inversed();
                const auto dx = k * (z - hx);

                const auto q = impl::make_unit_quaternion(dx / T{2});
                x_ = (q * x_).normalized();

                const auto i_kh = numerical::IDENTITY_MATRIX<3, T> - k * h;
                p_ = i_kh * p_ * i_kh.transposed() + k * r * k.transposed();
        }

public:
        void update_gyro(const numerical::Vector<3, T>& w, const T variance, const T dt)
        {
                predict(w, variance, dt);
        }

        void update_acc(const numerical::Vector<3, T>& a)
        {
                const T n = a.norm();
                if (!(n > 3))
                {
                        return;
                }

                const T sigma = T{0.1} * std::exp(std::abs(n - T{9.81}));
                update(a / n, {0, 0, 1}, sigma * sigma);
        }

        [[nodiscard]] numerical::Quaternion<T> attitude() const
        {
                return {x_.real(), x_.vec()};
        }
};
}
