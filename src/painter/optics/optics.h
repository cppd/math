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

/*
 Samuel R. Buss.
 3D Computer Graphics. A Mathematical Introduction with OpenGL.
 Cambridge University Press, 2003.

 Matt Pharr, Greg Humphreys.
 Physically Based Rendering. From theory to implementation. Second edition.
 Elsevier, 2010.

 The OpenGL® Shading Language.
*/

#pragma once

#include "com/math.h"
#include "com/vec.h"

namespace optics_implementation
{
template <typename T>
bool cos1_cos2(const Vector<3, T>& v, const Vector<3, T>& normal, T eta, T* cos1, T* cos2)
{
        T dot1 = dot(normal, v);

        // sin2 = eta * sin1
        T cos2_square = 1 - square(eta) * (1 - square(dot1));

        if (cos2_square > 0)
        {
                *cos1 = std::abs(dot1);
                *cos2 = std::sqrt(cos2_square);
                return true;
        }
        else
        {
                // полное внутреннее отражение
                return false;
        }
}
}

template <typename T>
Vector<3, T> reflect(const Vector<3, T>& v, const Vector<3, T>& normal)
{
        static_assert(std::is_floating_point_v<T>);

        return v - 2 * dot(v, normal) * normal;
}

// The OpenGL® Shading Language, Geometric Functions, Description.
template <typename T>
bool refract(const Vector<3, T>& v, const Vector<3, T>& normal, T eta, Vector<3, T>* t)
{
        static_assert(std::is_floating_point_v<T>);

        namespace impl = optics_implementation;

        T cos1, cos2;
        if (impl::cos1_cos2(v, normal, eta, &cos1, &cos2))
        {
                *t = v * eta - normal * (eta * dot(v, normal) + cos2);
                //*t = eta * (v  - normal * dot(v, normal)) - normal * cos2;
                return true;
        }
        return false;
}

// 3D Computer Graphics. A Mathematical Introduction with OpenGL.
// Для GCC работает в 2 раза медленнее функции из документации GLSL, где косинус
// второго угла определяется через косинус первого угла, а не через синус второго угла,
// как в этой функции.
template <typename T>
bool refract2(const Vector<3, T>& v, const Vector<3, T>& normal, T eta, Vector<3, T>* t)
{
        static_assert(std::is_floating_point_v<T>);

        Vector<3, T> t_lat = eta * (v - normal * dot(v, normal));
        T sin_square = dot(t_lat, t_lat);
        if (sin_square < 1)
        {
                *t = t_lat - normal * std::sqrt(1 - sin_square);
                return true;
        }
        return false;
}

// Physically Based Rendering, 8.2.1 Fresnel reflectance.
template <typename T>
bool fresnel_dielectric(const Vector<3, T>& v, const Vector<3, T>& normal, T n1, T n2, T* reflected, T* transmitted)
{
        static_assert(std::is_floating_point_v<T>);

        T cos1, cos2;

        if (!cos1_cos2(v, normal, n1 / n2, &cos1, &cos2))
        {
                *reflected = 1;
                *transmitted = 0;
                return false;
        }

        T r_parallel = (n2 * cos1 - n1 * cos2) / (n2 * cos1 + n1 * cos2);
        T r_perpendicular = (n1 * cos1 - n2 * cos2) / (n1 * cos1 + n2 * cos2);

        *reflected = T(0.5) * (square(r_parallel) + square(r_perpendicular));
        *transmitted = 1 - *reflected;

        return true;
}

// Physically Based Rendering, 8.2.1 Fresnel reflectance.
// η — the index of refraction of the conductor, k — its absorption coefficient.
template <typename T>
T fresnel_conductor(const Vector<3, T>& v, const Vector<3, T>& normal, T eta, T k)
{
        static_assert(std::is_floating_point_v<T>);

        T cos1 = std::abs(dot(normal, v));

        T two_eta_cos1 = 2 * eta * cos1;

        T t_parallel = (eta * eta + k * k) * cos1 * cos1 + 1;
        T r_parallel_square = (t_parallel - two_eta_cos1) / (t_parallel + two_eta_cos1);

        T t_perpendicular = eta * eta + k * k + cos1 * cos1;
        T r_perpendicular_square = (t_perpendicular - two_eta_cos1) / (t_perpendicular + two_eta_cos1);

        return T(0.5) * (r_parallel_square + r_perpendicular_square);
}
