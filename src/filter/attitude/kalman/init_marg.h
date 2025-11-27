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

#include "quaternion.h"

#include <src/numerical/vector.h>

#include <optional>

namespace ns::filter::attitude::kalman
{
template <typename T>
class InitMarg final
{
        using Vector3 = numerical::Vector<3, T>;

        const unsigned count_;

        unsigned acc_count_;
        Vector3 acc_data_;

        unsigned mag_count_;
        Vector3 mag_data_;

        void reset();

        [[nodiscard]] std::optional<Quaternion<T>> init();

public:
        explicit InitMarg(unsigned count);

        [[nodiscard]] std::optional<Quaternion<T>> update_acc(const Vector3& acc);

        [[nodiscard]] std::optional<Quaternion<T>> update_mag(const Vector3& mag);

        [[nodiscard]] std::optional<Quaternion<T>> update_acc_mag(const Vector3& acc, const Vector3& mag);
};
}
