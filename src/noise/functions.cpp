/*
Copyright (C) 2017-2024 Topological Manifold

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

#include "functions.h"

#include "noise.h"

#include <src/numerical/vector.h>
#include <src/settings/instantiation.h>

#include <cstddef>

namespace ns::noise
{
template <std::size_t N, typename T>
[[nodiscard]] T fractal_noise(const numerical::Vector<N, T>& p, const int count, const T lacunarity, const T gain)
{
        T sum = noise(p);

        T amplitude = gain;
        T frequency = lacunarity;
        T max = 1;
        for (int i = 1; i < count; ++i)
        {
                sum += amplitude * noise(p * frequency);
                max += amplitude;
                amplitude *= gain;
                frequency *= lacunarity;
        }

        return sum / max;
}

#define TEMPLATE(N, T) template T fractal_noise(const numerical::Vector<N, T>&, int, T, T);

TEMPLATE_INSTANTIATION_N_T_2(TEMPLATE)
}
