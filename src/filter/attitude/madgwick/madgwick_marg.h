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

#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

namespace ns::filter::attitude::madgwick
{
template <typename T>
class MadgwickMarg final
{
        using Vector3 = numerical::Vector<3, T>;

        numerical::Quaternion<T> q_{1, 0, 0, 0};
        T b_x_{1};
        T b_z_{0};
        Vector3 wb_{0, 0, 0};

public:
        bool update(const Vector3& w, const Vector3& a, const Vector3& m, T beta, T zeta, T dt);

        [[nodiscard]] const numerical::Quaternion<T>& attitude() const
        {
                return q_;
        }

        [[nodiscard]] const Vector3& bias() const
        {
                return wb_;
        }
};
}
