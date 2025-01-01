/*
Copyright (C) 2017-2025 Topological Manifold

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
Eric Heitz.
Sampling the GGX Distribution of Visible Normals.
Journal of Computer Graphics Techniques (JCGT), vol. 7, no. 4, 1–13, 2018.
*/

/*
Tomas Akenine-Möller, Eric Haines, Naty Hoffman,
Angelo Pesce, Michal Iwanicki, Sébastien Hillaire.
Real-Time Rendering. Fourth Edition.
CRC Press, 2018.

9.5 Fresnel Reflectance
9.6 Microgeometry
9.7 Microfacet Theory
9.8 BRDF Models for Surface Reflection
*/

#pragma once

#include "fresnel.h"

#include <src/com/exponent.h>
#include <src/com/interpolation.h>
#include <src/geometry/shapes/sphere_integral.h>
#include <src/numerical/complement.h>
#include <src/numerical/identity.h>
#include <src/numerical/optics.h>
#include <src/numerical/vector.h>
#include <src/sampling/pdf.h>
#include <src/sampling/sphere_uniform.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <tuple>

namespace ns::shading::ggx
{
namespace ggx_implementation
{
#if 0
template <typename T, typename RandomEngine>
[[nodiscard]] numerical::Vector<3, T> ggx_vn(RandomEngine& engine, const numerical::Vector<3, T>& ve, const T alpha)
{
        // Section 3.2: transforming the view direction to the hemisphere configuration
        const numerical::Vector<3, T> vh = numerical::Vector<3, T>(alpha * ve[0], alpha * ve[1], ve[2]).normalized();

        // Section 4.1: orthonormal basis (with special case if cross product is zero)
        const numerical::Vector<3, T> t0 = [&]
        {
                const T length_square = square(vh[0]) + square(vh[1]);
                if (length_square > 0)
                {
                        const T length = std::sqrt(length_square);
                        return numerical::Vector<3, T>(-vh[1] / length, vh[0] / length, 0);
                }
                return numerical::Vector<3, T>(1, 0, 0);
        }();
        const numerical::Vector<3, T> t1 = cross(vh, t0);

        // Section 4.2: parameterization of the projected area
        numerical::Vector<2, T> t = [&engine]
        {
                numerical::Vector<2, T> vector;
                T vector_length_square;
                sampling::uniform_in_sphere(engine, vector, vector_length_square);
                return vector;
        }();
        const T s = T{0.5} * (1 + vh[2]);
        t[1] = interpolation(std::sqrt(1 - square(t[0])), t[1], s);

        // Section 4.3: reprojection onto hemisphere
        const numerical::Vector<3, T> nh = [&]
        {
                const T z = sqrt_s(1 - dot(t, t));
                return t[0] * t0 + t[1] * t1 + z * vh;
        }();

        // Section 3.4: transforming the normal back to the ellipsoid configuration
        numerical::Vector<3, T> ne = numerical::Vector<3, T>(alpha * nh[0], alpha * nh[1], std::max(T{0}, nh[2])).normalized();

        return ne;
}
#else

template <std::size_t N, typename T>
[[nodiscard]] std::optional<numerical::Vector<N - 1, T>> plane_unit_vector(const numerical::Vector<N, T>& v)
{
        static_assert(N >= 2);

        T length_s = 0;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                length_s += square(v[i]);
        }

        if (length_s > 0)
        {
                const T length = std::sqrt(length_s);
                numerical::Vector<N - 1, T> res;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        res[i] = v[i] / length;
                }
                return res;
        }
        return std::nullopt;
}

// Section 4.1: orthonormal basis
template <std::size_t N, typename T>
[[nodiscard]] std::array<numerical::Vector<N, T>, N - 1> compute_orthonormal_basis(const numerical::Vector<N, T>& vh)
{
        std::array<numerical::Vector<N, T>, N - 1> res;

        if (const auto plane_v = plane_unit_vector(vh))
        {
                const std::array<numerical::Vector<N - 1, T>, N - 2> plane_basis =
                        numerical::orthogonal_complement_of_unit_vector(*plane_v);

                for (std::size_t i = 0; i < N - 2; ++i)
                {
                        for (std::size_t j = 0; j < N - 1; ++j)
                        {
                                res[i][j] = plane_basis[i][j];
                        }
                        res[i][N - 1] = 0;
                }
        }
        else
        {
                for (std::size_t i = 0; i < N - 2; ++i)
                {
                        res[i] = numerical::IDENTITY_ARRAY<N, T>[i];
                }
        }

        res[N - 2] = vh;
        res[N - 2] = numerical::orthogonal_complement(res);
        if (res[N - 2][N - 1] < 0)
        {
                res[N - 2] = -res[N - 2];
        }

        return res;
}

template <std::size_t N, typename T, typename RandomEngine>
[[nodiscard]] numerical::Vector<N, T> ggx_vn(RandomEngine& engine, const numerical::Vector<N, T>& ve, const T alpha)
{
        static_assert(N >= 3);

        // Section 3.2: transforming the view direction to the hemisphere configuration
        const numerical::Vector<N, T> vh = [&]
        {
                numerical::Vector<N, T> t;
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        t[i] = alpha * ve[i];
                }
                t[N - 1] = ve[N - 1];
                return t.normalized();
        }();

        // Section 4.1: orthonormal basis
        const std::array<numerical::Vector<N, T>, N - 1> orthonormal_basis = compute_orthonormal_basis(vh);

        // Section 4.2: parameterization of the projected area
        numerical::Vector<N - 1, T> t = [&engine]
        {
                numerical::Vector<N - 1, T> vector;
                T vector_length_square;
                sampling::uniform_in_sphere(engine, vector, vector_length_square);
                return vector;
        }();
        const T s = T{0.5} * (1 + vh[N - 1]);
        const T a = [&]
        {
                T sum = 0;
                for (std::size_t i = 0; i < N - 2; ++i)
                {
                        sum += square(t[i]);
                }
                return std::sqrt(1 - sum);
        }();
        t[N - 2] = interpolation(a, t[N - 2], s);

        // Section 4.3: reprojection onto hemisphere
        const numerical::Vector<N, T> nh = [&]
        {
                numerical::Vector<N, T> v = vh * sqrt_s(1 - dot(t, t));
                for (std::size_t i = 0; i < N - 1; ++i)
                {
                        v.multiply_add(t[i], orthonormal_basis[i]);
                }
                return v;
        }();

        // Section 3.4: transforming the normal back to the ellipsoid configuration
        numerical::Vector<N, T> ne;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                ne[i] = alpha * nh[i];
        }
        ne[N - 1] = std::max(T{0}, nh[N - 1]);

        return ne.normalized();
}

#endif

// (2), (9.37), (9.42)
template <typename T>
[[nodiscard]] T ggx_lambda(const T n_v, const T alpha)
{
        const T n_v_2 = square(n_v);
        const T t = square(alpha) * (1 - n_v_2) / n_v_2;

        return (std::sqrt(1 + t) - 1) / 2;
}

// (2), (9.24)
template <typename T>
[[nodiscard]] T ggx_g1(const T n_v, const T alpha)
{
        return 1 / (1 + ggx_lambda(n_v, alpha));
}

// (9.31)
template <typename T>
[[nodiscard]] T ggx_g2(const T n_v, const T n_l, const T alpha)
{
        return 1 / (1 + ggx_lambda(n_v, alpha) + ggx_lambda(n_l, alpha));
}
}

template <std::size_t N, typename T, typename RandomEngine>
[[nodiscard]] numerical::Vector<N, T> ggx_visible_normals_h(
        RandomEngine& engine,
        const numerical::Vector<N, T>& normal,
        const numerical::Vector<N, T>& v,
        const T alpha)
{
        static_assert(N >= 3);
        static_assert(std::is_floating_point_v<T>);

        namespace impl = ggx_implementation;

        std::array<numerical::Vector<N, T>, N - 1> basis = numerical::orthogonal_complement_of_unit_vector(normal);

        numerical::Vector<N, T> ve;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                ve[i] = dot(v, basis[i]);
        }
        ve[N - 1] = dot(v, normal);

        const numerical::Vector<N, T> ne = impl::ggx_vn(engine, ve, alpha);

        numerical::Vector<N, T> res = ne[N - 1] * normal;
        for (std::size_t i = 0; i < N - 1; ++i)
        {
                res.multiply_add(ne[i], basis[i]);
        }
        return res;
}

template <std::size_t N, typename T, typename RandomEngine>
[[nodiscard]] std::tuple<numerical::Vector<N, T>, numerical::Vector<N, T>> ggx_visible_normals_h_l(
        RandomEngine& engine,
        const numerical::Vector<N, T>& normal,
        const numerical::Vector<N, T>& v,
        const T alpha)
{
        const numerical::Vector<N, T> h = ggx_visible_normals_h(engine, normal, v, alpha);
        const numerical::Vector<N, T> l = numerical::reflect_vn(v, h);
        return {h, l};
}

// (1), (9.41)
template <std::size_t N, typename T>
[[nodiscard]] T ggx_d(const T n_h, const T alpha)
{
        static_assert(N >= 3);
        static_assert(std::is_floating_point_v<T>);

        if (n_h > 0)
        {
                static constexpr T K = geometry::shapes::SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE<N, T>;

                const T alpha_2 = square(alpha);
                const T v = 1 + square(n_h) * (alpha_2 - 1);
                // GGX<3> * pow(sin(hemisphere) / sin(ellipsoid), N - 3)
                //   sin(hemisphere) / sin(ellipsoid) = 1 / sqrt(v)
                // GGX<3> / pow(sqrt(v), N - 3)
                //   GGX<3> = n_h * alpha_2 / (K * v * v)
                // n_h * alpha_2 / (K * pow(v, 0.5 * (N + 1))
                T v_power = power<(N + 1) / 2>(v);
                if constexpr (((N + 1) & 1) == 1)
                {
                        v_power *= std::sqrt(v);
                }
                return alpha_2 / (K * v_power);
        }
        return 0;
}

// (3)
template <std::size_t N, typename T>
[[nodiscard]] T ggx_visible_normals_h_pdf(const T n_v, const T n_h, const T h_v, const T alpha)
{
        static_assert(N >= 3);
        static_assert(std::is_floating_point_v<T>);

        namespace impl = ggx_implementation;

        if (n_v > 0 && n_h > 0 && h_v > 0)
        {
                return impl::ggx_g1(n_v, alpha) * h_v * ggx_d<N>(n_h, alpha) / n_v;
        }
        return 0;
}

template <std::size_t N, typename T>
[[nodiscard]] T ggx_visible_normals_l_pdf(const T n_v, const T n_h, const T h_v, const T alpha)
{
        static_assert(N >= 3);
        static_assert(std::is_floating_point_v<T>);

        return sampling::reflected_pdf<N>(ggx_visible_normals_h_pdf<N>(n_v, n_h, h_v, alpha), h_v);
}

// (15), (18), (19)
// BRDF * (n · l) / PDF = Fresnel * G2 / G1
template <std::size_t N, typename T, typename Color>
[[nodiscard]] Color ggx_brdf(const T roughness, const Color& f0, const T n_v, const T n_l, const T n_h, const T h_l)
{
        static_assert(N >= 3);
        static_assert(std::is_floating_point_v<T>);

        namespace impl = ggx_implementation;

        if (n_v > 0 && n_l > 0 && h_l > 0)
        {
                const T alpha = square(roughness);

                const T d = ggx_d<N>(n_h, alpha);
                const T g2 = impl::ggx_g2(n_v, n_l, alpha);
                const T divisor = (n_v * n_l * (1 << (N - 1)) * power<N - 3>(h_l));

                return fresnel(f0, h_l) * (d * g2 / divisor);
        }

        return Color(0);
}
}
