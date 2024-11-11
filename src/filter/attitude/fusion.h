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
numerical::Matrix<3, 3, T> noise_covariance_matrix(const T variance, const T dt)
{
        return numerical::Matrix<3, 3, T>(variance * dt);
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

                x_ = first_order_quaternion_integrator(x_, w_last_, w, dt);
                w_last_ = w;

                const auto phi = impl::phi_matrix(w, dt);
                const auto q = impl::noise_covariance_matrix(variance, dt);

                p_ = phi * p_ * phi.transposed() + q;
        }

public:
        void update_gyro(const numerical::Vector<3, T>& w, const T variance, const T dt)
        {
                predict(w, variance, dt);
        }
};
}
