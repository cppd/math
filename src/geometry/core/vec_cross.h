/*
Copyright (C) 2017 Topological Manifold

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

#include "vec.h"

// Если векторы единичные, то это синус угла между векторами
// в двухмерном пространстве
template <typename T>
T cross(const Vector<2, T>& v0, const Vector<2, T>& v1)
{
        return v0[0] * v1[1] - v0[1] * v1[0];
}

// Дублирование кода из функции ortho_nn, но так удобнее, так как понятие
// векторное произведение имеется только в трёхмерном пространстве,
// в отличие от ортогональных дополнений
template <typename T>
Vector<3, T> cross(const Vector<3, T>& v0, const Vector<3, T>& v1)
{
        Vector<3, T> res;

        // clang-format off

        res[0] = +(v0[1] * v1[2] - v0[2] * v1[1]);
        res[1] = -(v0[0] * v1[2] - v0[2] * v1[0]);
        res[2] = +(v0[0] * v1[1] - v0[1] * v1[0]);

        // clang-format on

        return res;
}
