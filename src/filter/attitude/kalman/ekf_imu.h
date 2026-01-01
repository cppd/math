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

#pragma once

#include "quaternion.h"

#include <src/numerical/matrix.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T>
class EkfImu final
{
        using Vector3 = numerical::Vector<3, T>;
        using Matrix3 = numerical::Matrix<3, 3, T>;

        Quaternion<T> q_;
        Matrix3 p_;

        void predict(const Vector3& w0, const Vector3& w1, T variance, T dt);

        struct Update final
        {
                std::optional<Vector3> measurement;
                Vector3 reference_local;
                T variance;
        };

        template <std::size_t N>
        void update(const std::array<Update, N>& data);

public:
        EkfImu(const Quaternion<T>& q, T variance);

        void update_gyro(const Vector3& w0, const Vector3& w1, T variance, T dt);

        void update_z(const Vector3& z, T z_variance, T y_variance);

        [[nodiscard]] const Quaternion<T>& attitude() const
        {
                return q_;
        }
};
}
