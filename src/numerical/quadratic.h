/*
Copyright (C) 2017-2019 Topological Manifold

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

#include "com/math.h"

namespace numerical
{
template <typename T>
bool quadratic_equation(T a, T b, T c, T* r1, T* r2)
{
        T discriminant = b * b - 4 * a * c;

        if (discriminant < 0)
        {
                return false;
        }

        T sqrt_d = any_sqrt(discriminant);

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
