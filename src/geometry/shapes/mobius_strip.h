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
#include <src/numerical/quaternion.h>
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
        x = std::copysign(std::pow(std::abs(x), 5), x);
        x = (x + 1) / 2;

        return PI<T> * x;
}
}

template <typename T, typename RandomEngine>
Vector<3, T> mobius_strip_point(T width, RandomEngine& random_engine)
{
        namespace impl = mobius_strip_implementation;

        std::uniform_real_distribution<T> urd_alpha(0, 2 * PI<T>);
        std::uniform_real_distribution<T> urd_line(-width / 2, width / 2);

        T alpha = urd_alpha(random_engine);

        // Случайная точка вдоль Z, вращение вокруг Y, смещение по X и вращение вокруг Z
        Vector<3, T> v(0, 0, urd_line(random_engine));
        v = rotate_vector(Vector<3, T>(0, 1, 0), PI<T> / 2 - impl::curve(alpha), v);
        v += Vector<3, T>(1, 0, 0);
        v = rotate_vector(Vector<3, T>(0, 0, 1), alpha, v);

        return v;
}
}
