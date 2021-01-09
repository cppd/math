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

#include "../objects.h"

#include <src/color/color.h>
#include <src/numerical/vec.h>
#include <src/sampling/sphere_cosine.h>

namespace ns::painter
{
inline std::tuple<Color::DataType, Color::DataType, Color::DataType> prepare_shading_parameters(
        Color::DataType diffuse,
        Color::DataType specular,
        Color::DataType specular_power)
{
        diffuse = std::max(Color::DataType(0), diffuse);
        specular = std::max(Color::DataType(0), specular);
        const Color::DataType sum = diffuse + specular;
        if (sum != 0)
        {
                diffuse /= sum;
                specular /= sum;
        }
        else
        {
                diffuse = 0.5;
                specular = 0.5;
        }

        specular_power = std::max(Color::DataType(1), specular_power);

        return {diffuse, specular, specular_power};
}

template <std::size_t N, typename T>
Color surface_lighting(
        const Vector<N, T>& dir_to_light,
        const Vector<N, T>& normal,
        const Vector<N, T>& dir_reflection,
        const Color& color,
        T diffuse,
        T specular,
        T specular_power)
{
        Color c = (diffuse * dot(normal, dir_to_light)) * color;

        T specular_dot = dot(dir_reflection, dir_to_light);
        if (specular_dot > 0)
        {
                c += Color(specular * std::pow(std::min(specular_dot, T(1)), specular_power));
        }

        return c;
}

template <std::size_t N, typename T, typename RandomEngine>
SurfaceReflection<N, T> surface_ray_direction(
        const Vector<N, T>& normal,
        const Vector<N, T>& dir_reflection,
        const Color& color,
        T diffuse,
        T specular_power,
        RandomEngine& engine)
{
        if (std::uniform_real_distribution<T>(0, 1)(engine) < diffuse)
        {
                return {color, sampling::cosine_weighted_on_hemisphere(engine, normal)};
        }
        return {Color(1), sampling::power_cosine_weighted_on_hemisphere(engine, dir_reflection, specular_power)};
}
}
