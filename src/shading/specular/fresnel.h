/*
Copyright (C) 2017-2026 Topological Manifold

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
Matt Pharr, Wenzel Jakob, Greg Humphreys.
Physically Based Rendering. From theory to implementation. Third edition.
Elsevier, 2017.

8.2.1 Fresnel reflectance
*/

#pragma once

#include <src/com/exponent.h>
#include <src/numerical/vector.h>

#include <array>
#include <cstddef>
#include <optional>
#include <type_traits>

namespace ns::shading::specular
{
namespace fresnel_implementation
{
template <std::size_t N, typename T>
std::optional<std::array<T, 2>> cos1_cos2(
        const numerical::Vector<N, T>& v,
        const numerical::Vector<N, T>& normal,
        const T eta)
{
        const T dot1 = dot(normal, v);

        // sin2 = eta * sin1
        const T cos2_square = 1 - square(eta) * (1 - square(dot1));

        if (cos2_square > 0)
        {
                return {
                        {std::abs(dot1), std::sqrt(cos2_square)}
                };
        }

        // total internal reflection
        return std::nullopt;
}
}

template <typename T>
struct FresnelDielectric final
{
        T reflected;
        T transmitted;

        FresnelDielectric(const std::type_identity_t<T> reflected, const std::type_identity_t<T> transmitted)
                : reflected(reflected),
                  transmitted(transmitted)
        {
        }
};

template <std::size_t N, typename T>
std::optional<FresnelDielectric<T>> fresnel_dielectric(
        const numerical::Vector<N, T>& v,
        const numerical::Vector<N, T>& normal,
        const T n1,
        const T n2)
{
        static_assert(std::is_floating_point_v<T>);

        namespace impl = fresnel_implementation;

        const std::optional<std::array<T, 2>> cos1_cos2 = impl::cos1_cos2(v, normal, n1 / n2);
        if (!cos1_cos2)
        {
                return std::nullopt;
        }

        const auto [cos1, cos2] = *cos1_cos2;

        const T r_parallel = (n2 * cos1 - n1 * cos2) / (n2 * cos1 + n1 * cos2);
        const T r_perpendicular = (n1 * cos1 - n2 * cos2) / (n1 * cos1 + n2 * cos2);

        const T reflected = T{0.5} * (square(r_parallel) + square(r_perpendicular));
        const T transmitted = 1 - reflected;

        return {
                {reflected, transmitted}
        };
}

// Physically Based Rendering, 8.2.1 Fresnel reflectance.
// η — the index of refraction of the conductor, k — its absorption coefficient.
template <std::size_t N, typename T>
T fresnel_conductor(const numerical::Vector<N, T>& v, const numerical::Vector<N, T>& normal, const T eta, const T k)
{
        static_assert(std::is_floating_point_v<T>);

        const T cos1 = std::abs(dot(normal, v));

        const T two_eta_cos1 = 2 * eta * cos1;

        const T t_parallel = (eta * eta + k * k) * cos1 * cos1 + 1;
        const T r_parallel_square = (t_parallel - two_eta_cos1) / (t_parallel + two_eta_cos1);

        const T t_perpendicular = eta * eta + k * k + cos1 * cos1;
        const T r_perpendicular_square = (t_perpendicular - two_eta_cos1) / (t_perpendicular + two_eta_cos1);

        return T{0.5} * (r_parallel_square + r_perpendicular_square);
}
}
