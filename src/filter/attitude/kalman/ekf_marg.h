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

#pragma once

#include "init_marg.h"
#include "quaternion.h"

#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T>
class EkfMarg final
{
        using Vector3 = numerical::Vector<3, T>;
        using Vector6 = numerical::Vector<6, T>;
        using Matrix3 = numerical::Matrix<3, 3, T>;
        using Matrix6 = numerical::Matrix<6, 6, T>;

        InitMarg<T> init_;

        std::optional<Quaternion<T>> q_;
        Vector3 b_{0};
        Matrix6 p_{numerical::ZERO_MATRIX};

        void predict(const Vector3& w0, const Vector3& w1, T variance_r, T variance_w, T dt);

        struct Update final
        {
                std::optional<Vector3> measurement;
                Vector3 reference_local;
                T variance;
        };

        template <std::size_t N>
        void update(const std::array<Update, N>& data);

public:
        void update_gyro(const Vector3& w0, const Vector3& w1, T variance_r, T variance_w, T dt);

        bool update_acc(const Vector3& a, T variance, T variance_direction);

        bool update_mag(const Vector3& m, T variance, T variance_direction);

        bool update_acc_mag(const Vector3& a, const Vector3& m, T a_variance, T m_variance);

        [[nodiscard]] std::optional<numerical::Quaternion<T>> attitude() const
        {
                if (q_)
                {
                        return numerical::Quaternion<T>(*q_);
                }
                return std::nullopt;
        }

        [[nodiscard]] const Vector3& bias() const
        {
                return b_;
        }
};
}
