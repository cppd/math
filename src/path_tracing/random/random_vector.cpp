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

#include "random_vector.h"

#include "com/math.h"

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
                if (length_square > 0 && length_square <= 1 && dot(v, normal) > 0)
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

// Physically Based Rendering
// 13.6.2 SAMPLING A UNIT DISK
vec2 random_disk_any_length(std::mt19937_64& engine)
{
        std::uniform_real_distribution<float> urf(0, 1);
        float r = std::sqrt(urf(engine));
        float theta = 2 * static_cast<float>(PI) * urf(engine);
        return vec2(r * std::cos(theta), r * std::sin(theta));
}

namespace
{
// Physically Based Rendering
// 13.6.3 COSINE-WEIGHTED HEMISPHERE SAMPLING
template <typename FLOAT>
vec3 random_hemisphere_cosine_any_length(std::mt19937_64& engine, const vec3& normal)
{
        // Два единичных перпендикулярных вектора, перпендикулярных нормали.
        vec3 non_collinear_vector = std::abs(normal[0]) > 0.5 ? vec3(0, 1, 0) : vec3(1, 0, 0);
        vec3 e0 = normalize(cross(normal, non_collinear_vector));
        vec3 e1 = cross(normal, e0);

        std::uniform_real_distribution<FLOAT> urf(0, 1);

        FLOAT r_square = urf(engine);
        FLOAT theta = 2 * static_cast<FLOAT>(PI) * urf(engine);
        FLOAT r = std::sqrt(r_square);

        // Точки равномерно размещаются внутри круга на плоскости, затем проецируются на полусферу.
        double x = r * std::cos(theta);
        double y = r * std::sin(theta);
        double z = std::sqrt(std::max(static_cast<FLOAT>(0), 1 - r_square));

        // Вектор почти единичный, но тут не важно, единичный он или нет.
        return x * e0 + y * e1 + z * normal;
}
}

vec3 random_hemisphere_cosine_any_length(std::mt19937_64& engine, const vec3& normal)
{
        // Тип float быстрее работает для функций sin, cos и sqrt, чем тип double.
        return random_hemisphere_cosine_any_length<float>(engine, normal);
}
