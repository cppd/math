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

#include "ggx_diffuse.h"
#include "lambertian.h"

#include "../objects.h"

#include <src/com/error.h>

namespace ns::painter
{
template <std::size_t N, typename T>
Color shade(
        const Color::DataType alpha,
        const T metalness,
        const T roughness,
        const Color& color,
        const Vector<N, T>& n,
        const Vector<N, T>& v,
        const Vector<N, T>& l)
{
        if (alpha <= 0)
        {
                return Color(0);
        }
        if constexpr (N == 3)
        {
                return alpha * GGXDiffuse<T>::shade(metalness, roughness, color, n, v, l);
        }
        else
        {
                return alpha * Lambertian<N, T>::shade(color, n, l);
        }
}

template <std::size_t N, typename T, typename RandomEngine>
ShadeSample<N, T> sample_shade(
        RandomEngine& random_engine,
        const ShadeType shade_type,
        const Color::DataType alpha,
        const T metalness,
        const T roughness,
        const Color& color,
        const Vector<N, T>& n,
        const Vector<N, T>& v)
{
        switch (shade_type)
        {
        case ShadeType::Reflection:
        {
                if (alpha <= 0)
                {
                        ShadeSample<N, T> s;
                        s.color = Color(0);
                        s.l = Vector<N, T>(0);
                }
                ShadeSample<N, T> s;
                if constexpr (N == 3)
                {
                        std::tie(s.l, s.color) =
                                GGXDiffuse<T>::sample_shade(random_engine, metalness, roughness, color, n, v);
                }
                else
                {
                        std::tie(s.l, s.color) = Lambertian<N, T>::sample_shade(random_engine, color, n);
                }
                s.color *= alpha;
                return s;
        }
        case ShadeType::Transmission:
        {
                ShadeSample<N, T> s;
                s.l = -v;
                s.color = Color(1 - alpha);
                return s;
        }
        }
        error_fatal("Unknown shade type " + std::to_string(static_cast<unsigned long long>(shade_type)));
}
}
