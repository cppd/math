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

// Samuel R. Buss.
// 3D Computer Graphics. A Mathematical Introduction with OpenGL.
// Cambridge University Press, 2003.
// --
// Matt Pharr, Greg Humphreys.
// Physically Based Rendering. From theory to implementation. Second edition.
// Elsevier, 2010.
// --
// The OpenGL® Shading Language.

#include "optics.h"

namespace
{
bool cos1_cos2(const vec3& v, const vec3& normal, double eta, double* cos1, double* cos2)
{
        double dot1 = dot(normal, v);

        // sin2 = eta * sin1
        double cos2_square = 1 - square(eta) * (1 - square(dot1));

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

vec3 reflect(const vec3& v, const vec3& normal)
{
        return v - 2 * dot(v, normal) * normal;
}

// The OpenGL® Shading Language, Geometric Functions, Description.
bool refract(const vec3& v, const vec3& normal, double eta, vec3* t)
{
        double cos1, cos2;
        if (cos1_cos2(v, normal, eta, &cos1, &cos2))
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
bool refract2(const vec3& v, const vec3& normal, double eta, vec3* t)
{
        vec3 t_lat = eta * (v - normal * dot(v, normal));
        double sin_square = dot(t_lat, t_lat);
        if (sin_square < 1)
        {
                *t = t_lat - normal * std::sqrt(1 - sin_square);
                return true;
        }
        return false;
}

// Physically Based Rendering, 8.2.1 Fresnel reflectance.
bool fresnel_dielectric(const vec3& v, const vec3& normal, double n1, double n2, double* reflected, double* transmitted)
{
        double cos1, cos2;

        if (!cos1_cos2(v, normal, n1 / n2, &cos1, &cos2))
        {
                *reflected = 1.0;
                *transmitted = 0.0;
                return false;
        }

        double r_parallel = (n2 * cos1 - n1 * cos2) / (n2 * cos1 + n1 * cos2);
        double r_perpendicular = (n1 * cos1 - n2 * cos2) / (n1 * cos1 + n2 * cos2);

        *reflected = 0.5 * (square(r_parallel) + square(r_perpendicular));
        *transmitted = 1.0 - *reflected;

        return true;
}

// Physically Based Rendering, 8.2.1 Fresnel reflectance.
// η — the index of refraction of the conductor, k — its absorption coefficient.
double fresnel_conductor(const vec3& v, const vec3& normal, double eta, double k)
{
        double cos1 = std::abs(dot(normal, v));

        double two_eta_cos1 = 2 * eta * cos1;

        double t_parallel = (eta * eta + k * k) * cos1 * cos1 + 1;
        double r_parallel_square = (t_parallel - two_eta_cos1) / (t_parallel + two_eta_cos1);

        double t_perpendicular = eta * eta + k * k + cos1 * cos1;
        double r_perpendicular_square = (t_perpendicular - two_eta_cos1) / (t_perpendicular + two_eta_cos1);

        return 0.5 * (r_parallel_square + r_perpendicular_square);
}
