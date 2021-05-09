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

#include <src/shading/ggx_diffuse.h>
#include <src/shading/lambertian.h>

namespace ns::painter
{
template <std::size_t N, typename T>
struct ShapeBRDF final
{
        static Color f(
                const T metalness,
                const T roughness,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& v,
                const Vector<N, T>& l)
        {
                if constexpr (N == 3)
                {
                        return shading::GGXDiffuseBRDF<T>::f(metalness, roughness, color, n, v, l);
                }
                else
                {
                        return shading::LambertianBRDF<N, T>::f(color, n, l);
                }
        }

        template <typename RandomEngine>
        static shading::Sample<N, T> sample_f(
                RandomEngine& random_engine,
                const T metalness,
                const T roughness,
                const Color& color,
                const Vector<N, T>& n,
                const Vector<N, T>& v)
        {
                if constexpr (N == 3)
                {
                        return shading::GGXDiffuseBRDF<T>::sample_f(random_engine, metalness, roughness, color, n, v);
                }
                else
                {
                        return shading::LambertianBRDF<N, T>::sample_f(random_engine, color, n);
                }
        }
};
}
