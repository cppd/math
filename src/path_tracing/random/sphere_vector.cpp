/*
Copyright (C) 2017, 2018 Topological Manifold

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

#include "sphere_vector.h"

#include "com/random/vector.h"
#include "geometry/core/complement.h"

#include "com/math.h"

#if 0

// Для получения равномерных точек на сфере можно ещё использовать std::normal_distribution
// с последующим делением на длину получаемого вектора, но для трёхмерных пространств это
// работает медленнее, чем простой метод с выбрасыванием значений.
//
// Методы с синусами и косинусами из книги Physically Based Rendering тоже работают
// медленее этих методов.

template <size_t N, typename T>
Vector<N, T> random_sphere_any_length(std::mt19937_64& engine)
{
        std::uniform_real_distribution<T> urd(-1.0, 1.0);

        for (;;)
        {
                Vector<N, T> v = random_vector<N, T>(engine, urd);
                T length_square = dot(v, v);
                if (length_square <= 1 && length_square > 0)
                {
                        return v;
                }
        }
}

template <size_t N, typename T>
Vector<N, T> random_hemisphere_any_length(std::mt19937_64& engine, const Vector<N, T>& normal)
{
        Vector<N, T> v = random_sphere_any_length<N, T>(engine);

        return (dot(v, normal) >= 0) ? v : -v;
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

#endif

namespace
{
// Physically Based Rendering
// 13.6.3 COSINE-WEIGHTED HEMISPHERE SAMPLING

#if 0
template <typename T>
Vector<3, T> random_hemisphere_cosine_any_length_by_sincos(std::mt19937_64& engine, const Vector<3, T>& normal)
{
        // Два единичных перпендикулярных вектора, перпендикулярных нормали.
        std::array<Vector<3, T>, 2> basis = orthogonal_complement_of_unit_vector(normal);

        std::uniform_real_distribution<T> urd(0, 1);

        T r_square = urd(engine);
        T theta = 2 * static_cast<T>(PI) * urd(engine);
        T r = std::sqrt(r_square);

        // Точки равномерно размещаются внутри круга на плоскости, затем проецируются на полусферу.
        T x = r * std::cos(theta);
        T y = r * std::sin(theta);

        T z = std::sqrt(std::max(static_cast<T>(0), 1 - r_square));

        // Вектор почти единичный, но тут не важно, единичный он или нет.
        return x * basis[0] + y * basis[1] + z * normal;
}
#endif

template <typename T>
Vector<3, T> random_hemisphere_cosine_any_length_by_rejection(std::mt19937_64& engine, const Vector<3, T>& normal)
{
        // Два единичных перпендикулярных вектора, перпендикулярных нормали.
        std::array<Vector<3, T>, 2> basis = orthogonal_complement_of_unit_vector(normal);

        std::uniform_real_distribution<T> urd(-1, 1);
        Vector<2, T> v;
        T r_square;
        for (;;)
        {
                v = random_vector<2, T>(engine, urd);
                r_square = dot(v, v);
                if (r_square <= 1 && r_square > 0)
                {
                        break;
                }
        }

        T z = std::sqrt(std::max(static_cast<T>(0), 1 - r_square));

        // Вектор почти единичный, но тут не важно, единичный он или нет.
        return v[0] * basis[0] + v[1] * basis[1] + z * normal;
}
}

vec3 random_hemisphere_cosine_any_length(std::mt19937_64& engine, const vec3& normal)
{
        return to_vector<double>(random_hemisphere_cosine_any_length_by_rejection(engine, to_vector<float>(normal)));
}
