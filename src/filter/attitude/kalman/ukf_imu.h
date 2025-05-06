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

#include "init_imu.h"
#include "quaternion.h"

#include <src/filter/core/sigma_points.h>
#include <src/numerical/matrix.h>
#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <optional>
#include <type_traits>

namespace ns::filter::attitude::kalman
{
template <typename T>
class UkfImu final
{
        using Vector3 = numerical::Vector<3, T>;
        using Matrix3 = numerical::Matrix<3, 3, T>;

        static constexpr std::size_t POINT_COUNT =
                std::tuple_size_v<std::remove_cvref_t<decltype(std::declval<core::SigmaPoints<3, T>>().wm())>>;

        static_assert(POINT_COUNT >= 2 * 3 + 1);

        const core::SigmaPoints<3, T> sigma_points_;

        InitImu<T> init_;

        std::optional<Quaternion<T>> q_;
        std::array<Vector3, POINT_COUNT> propagated_points_;
        std::array<Quaternion<T>, POINT_COUNT> propagated_quaternions_;
        Vector3 x_;
        Matrix3 p_;

        bool predicted_{false};

        void predict(const Vector3& w0, const Vector3& w1, T variance, T dt);

        struct Update final
        {
                std::optional<Vector3> measurement;
                Vector3 reference;
                T variance;
        };

        template <std::size_t N>
        void update(const std::array<Update, N>& data);

public:
        explicit UkfImu(T variance);

        void update_gyro(const Vector3& w0, const Vector3& w1, T variance, T dt);

        bool update_acc(const Vector3& a, T variance, T variance_direction);

        [[nodiscard]] std::optional<numerical::Quaternion<T>> attitude() const
        {
                if (q_)
                {
                        return numerical::Quaternion<T>(*q_);
                }
                return std::nullopt;
        }
};
}
