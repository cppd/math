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

#include <cmath>

namespace ns::numerical
{
template <typename T>
[[nodiscard]] bool quadratic_equation(const T& a, const T& b, const T& c, T* const r1, T* const r2)
{
        static_assert(std::is_floating_point_v<T>);

        const T discriminant = b * b - 4 * a * c;

        if (discriminant < 0)
        {
                return false;
        }

        const T sqrt_d = std::sqrt(discriminant);

        if (b >= 0)
        {
                *r1 = (-b - sqrt_d) / (2 * a);
                *r2 = 2 * c / (-b - sqrt_d);
        }
        else
        {
                *r1 = (-b + sqrt_d) / (2 * a);
                *r2 = 2 * c / (-b + sqrt_d);
        }

        return true;
}
}
