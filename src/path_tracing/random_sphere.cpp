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

#include "random_sphere.h"

#include "constants.h"

// Для получения равномерных точек на сфере можно ещё использовать std::normal_distribution
// с последующим делением на длину получаемого вектора, но для трёхмерных пространств это
// работает медленнее, чем простой метод с выбрасыванием значений.
//
// Методы с синусами и косинусами из книги Physically Based Rendering тоже работают
// медленее этих методов.

vec3 random_hemisphere_any_length(std::mt19937_64& engine, const vec3& normal)
{
        std::uniform_real_distribution<double> urd(-1.0, 1.0);

        for (;;)
        {
                vec3 v(urd(engine), urd(engine), urd(engine));
                double length_square = dot(v, v);
                if (length_square > 0 && length_square <= 1 && dot(v, normal) > EPSILON)
                {
                        return v;
                }
        }
}

vec3 random_sphere_any_length(std::mt19937_64& engine)
{
        std::uniform_real_distribution<double> urd(-1.0, 1.0);

        for (;;)
        {
                vec3 v(urd(engine), urd(engine), urd(engine));
                double length_square = dot(v, v);
                if (length_square > 0 && length_square <= 1)
                {
                        return v;
                }
        }
}
