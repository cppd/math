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

#include <src/numerical/quaternion.h>
#include <src/numerical/vector.h>

namespace ns::filter::attitude::madgwick
{
template <typename T>
class MadgwickImu final
{
        using Vector3 = numerical::Vector<3, T>;

        numerical::Quaternion<T> q_{numerical::IDENTITY_QUATERNION};

public:
        void update(const Vector3& w, const Vector3& acc, T beta, T dt);

        [[nodiscard]] const numerical::Quaternion<T>& attitude() const
        {
                return q_;
        }
};
}
