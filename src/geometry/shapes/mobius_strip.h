/*
Copyright (C) 2017-2021 Topological Manifold

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

#include <src/com/constant.h>
#include <src/com/math.h>
#include <src/numerical/vec.h>

#include <cmath>
#include <random>

namespace ns::geometry
{
namespace mobius_strip_implementation
{
// На входе от 0 до 2 * PI, на выходе от 0 до PI
template <typename T>
T curve(T x)
{
        x = x / (2 * PI<T>);

        x = 2 * x - 1;
        x = std::copysign(power<5>(std::abs(x)), x);
        x = (x + 1) / 2;

        return PI<T> * x;
}
}

template <typename T, typename RandomEngine>
Vector<3, T> mobius_strip_point(T width, RandomEngine& random_engine)
{
        namespace impl = mobius_strip_implementation;

        const T alpha = std::uniform_real_distribution<T>(0, 2 * PI<T>)(random_engine);
        const T curve_angle = PI<T> / 2 - impl::curve(alpha);

        // Случайная точка вдоль Z
        Vector<3, T> v(0, 0, std::uniform_real_distribution<T>(-width / 2, width / 2)(random_engine));

        // Вращение вокруг Y
        v = Vector<3, T>(v[2] * std::sin(curve_angle), 0, v[2] * std::cos(curve_angle));

        // Смещение по X
        v = Vector<3, T>(v[0] + 1, 0, v[2]);

        // Вращение вокруг Z
        v = Vector<3, T>(v[0] * std::cos(alpha), v[0] * std::sin(alpha), v[2]);

        return v;
}
}
