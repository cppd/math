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

#include "ggx.h"
#include "lambertian.h"

namespace ns::painter
{
template <std::size_t N, typename T>
Color shade(
        T metalness,
        T roughness,
        const Color& color,
        const Vector<N, T>& n,
        const Vector<N, T>& v,
        const Vector<N, T>& l)
{
        if constexpr (N == 3)
        {
                return GGX<T>::shade(metalness, roughness, color, n, v, l);
        }
        else
        {
                return Lambertian<N, T>::shade(color, n, l);
        }
}

template <std::size_t N, typename T, typename RandomEngine>
std::tuple<Vector<N, T>, Color> sample_shade(
        RandomEngine& random_engine,
        T metalness,
        T roughness,
        const Color& color,
        const Vector<N, T>& n,
        const Vector<N, T>& v)
{
        if constexpr (N == 3)
        {
                return GGX<T>::sample_shade(random_engine, metalness, roughness, color, n, v);
        }
        else
        {
                return Lambertian<N, T>::sample_shade(random_engine, color, n);
        }
}
}
