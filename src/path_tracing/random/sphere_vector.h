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

#pragma once

#include "com/random/vector.h"
#include "com/vec.h"
#include "geometry/core/complement.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <type_traits>

// Physically Based Rendering.
// 13.6.2 SAMPLING A UNIT DISK.
// 13.6.3 COSINE-WEIGHTED HEMISPHERE SAMPLING.
// Берётся два единичных перпендикулярных вектора, перпендикулярных нормали.
// Точки равномерно размещаются внутри круга на плоскости этих векторов,
// затем проецируются на полусферу параллельно нормали.

template <typename RandomEngine, typename T>
Vector<3, T> random_cosine_hemisphere_any_length(RandomEngine& random_engine, const Vector<3, T>& normal)
{
        Vector<2, T> v;
        T r_square;

#if 0
        // Работает медленее алгоритма с выбрасыванием значений

        std::uniform_real_distribution<T> urd(0, 1);

        r_square = urd(random_engine);
        T theta = 2 * static_cast<T>(PI) * urd(random_engine);
        T r = std::sqrt(r_square);
        v[0] = r * std::cos(theta);
        v[1] = r * std::sin(theta);
#else
        // Работает быстрее алгоритма с синусами и косинусами

        std::uniform_real_distribution<T> urd(-1, 1);

        while (true)
        {
                v = random_vector<2, T>(random_engine, urd);
                r_square = dot(v, v);
                if (r_square <= 1 && r_square > 0)
                {
                        break;
                }
        }
#endif

        T z = std::sqrt(std::max(static_cast<T>(0), 1 - r_square));

        std::array<Vector<3, T>, 2> basis = orthogonal_complement_of_unit_vector(normal);

        return v[0] * basis[0] + v[1] * basis[1] + z * normal;
}

#if 0

// Для получения равномерных точек на сфере можно ещё использовать std::normal_distribution
// с последующим делением на длину получаемого вектора, но для трёхмерных пространств это
// работает медленнее, чем простой метод с выбрасыванием значений.
// Методы с синусами и косинусами из книги Physically Based Rendering тоже работают
// медленее этих методов.

template <size_t N, typename T, typename RandomEngine>
Vector<N, T> random_sphere_any_length(RandomEngine& random_engine)
{
        std::uniform_real_distribution<T> urd(-1.0, 1.0);

        while (true)
        {
                Vector<N, T> v = random_vector<N, T>(random_engine, urd);
                T length_square = dot(v, v);
                if (length_square <= 1 && length_square > 0)
                {
                        return v;
                }
        }
}

template <size_t N, typename T, typename RandomEngine>
Vector<N, T> random_hemisphere_any_length(RandomEngine& random_engine, const Vector<N, T>& normal)
{
        Vector<N, T> v = random_sphere_any_length<N, T>(random_engine);

        return (dot(v, normal) >= 0) ? v : -v;
}

#endif
