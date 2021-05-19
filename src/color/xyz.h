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

/*
Chris Wyman, Peter-Pike Sloan, Peter Shirley.
Simple Analytic Approximations to the CIE XYZ Color Matching Functions.
Journal of Computer Graphics Techniques, Vol. 2, No. 2, 2013.
*/

#pragma once

#include <src/com/error.h>

#include <cmath>

namespace ns::color
{
namespace xyz_implementation
{
template <typename T>
T g(T wave, T m, T t1, T t2)
{
        T v = (wave - m) * (wave < m ? t1 : t2);
        return std::exp(T(-0.5) * v * v);
}

// Integrate[Exp[-1/2*(t*(x-m))^2],{x,a,b}]
template <typename T>
T g_integral(T wave_1, T wave_2, T m, T t)
{
        // sqrt(2)
        static constexpr T SQRT_2 = 1.4142135623730950488016887242096980785696718753769L;
        // sqrt(PI/2)
        static constexpr T SQRT_PI_2 = 1.2533141373155002512078826424055226265034933703050L;

        const T ts = t / SQRT_2;
        return (SQRT_PI_2 / t) * (std::erf(ts * (wave_2 - m)) - std::erf(ts * (wave_1 - m)));
}

template <typename T>
T g_integral(T wave_1, T wave_2, T m, T t1, T t2)
{
        ASSERT(wave_1 < wave_2);
        if (wave_2 <= m)
        {
                return g_integral(wave_1, wave_2, m, t1);
        }
        if (wave_1 >= m)
        {
                return g_integral(wave_1, wave_2, m, t2);
        }
        return g_integral(wave_1, m, m, t1) + g_integral(m, wave_2, m, t2);
}

//

template <typename T>
T x_31(T wave)
{
        static_assert(std::is_floating_point_v<T>);

        T g1 = g(wave, T(442.0), T(0.0624), T(0.0374));
        T g2 = g(wave, T(599.8), T(0.0264), T(0.0323));
        T g3 = g(wave, T(501.1), T(0.0490), T(0.0382));
        return T(0.362) * g1 + T(1.056) * g2 + T(-0.065) * g3;
}

template <typename T>
T x_31_integral(T wave_1, T wave_2)
{
        static_assert(std::is_floating_point_v<T>);

        T g1 = g_integral(wave_1, wave_2, T(442.0), T(0.0624), T(0.0374));
        T g2 = g_integral(wave_1, wave_2, T(599.8), T(0.0264), T(0.0323));
        T g3 = g_integral(wave_1, wave_2, T(501.1), T(0.0490), T(0.0382));
        return T(0.362) * g1 + T(1.056) * g2 + T(-0.065) * g3;
}

//

template <typename T>
T y_31(T wave)
{
        static_assert(std::is_floating_point_v<T>);

        T g1 = g(wave, T(568.8), T(0.0213), T(0.0247));
        T g2 = g(wave, T(530.9), T(0.0613), T(0.0322));
        return T(0.821) * g1 + T(0.286) * g2;
}

template <typename T>
T y_31_integral(T wave_1, T wave_2)
{
        static_assert(std::is_floating_point_v<T>);

        T g1 = g_integral(wave_1, wave_2, T(568.8), T(0.0213), T(0.0247));
        T g2 = g_integral(wave_1, wave_2, T(530.9), T(0.0613), T(0.0322));
        return T(0.821) * g1 + T(0.286) * g2;
}

//

template <typename T>
T z_31(T wave)
{
        static_assert(std::is_floating_point_v<T>);

        T g1 = g(wave, T(437.0), T(0.0845), T(0.0278));
        T g2 = g(wave, T(459.0), T(0.0385), T(0.0725));
        return T(1.217) * g1 + T(0.681) * g2;
}

template <typename T>
T z_31_integral(T wave_1, T wave_2)
{
        static_assert(std::is_floating_point_v<T>);

        T g1 = g_integral(wave_1, wave_2, T(437.0), T(0.0845), T(0.0278));
        T g2 = g_integral(wave_1, wave_2, T(459.0), T(0.0385), T(0.0725));
        return T(1.217) * g1 + T(0.681) * g2;
}

//

template <typename T>
T x_64(T wave)
{
        static_assert(std::is_floating_point_v<T>);

        T t1 = std::log((wave + T(570.1)) / T(1014));
        T t2 = std::log((T(1338) - wave) / T(743.5));
        return T(0.398) * std::exp(T(-1250) * t1 * t1) + T(1.132) * std::exp(T(-234) * t2 * t2);
}

template <typename T>
T y_64(T wave)
{
        static_assert(std::is_floating_point_v<T>);

        T t = (wave - T(556.1)) / T(46.14);
        return T(1.011) * std::exp(T(-0.5) * t * t);
}

template <typename T>
T z_64(T wave)
{
        static_assert(std::is_floating_point_v<T>);

        T t = std::log((wave - T(265.8)) / T(180.4));
        return T(2.060) * std::exp(T(-32) * t * t);
}
}

enum XYZ
{
        XYZ_31,
        XYZ_64
};

template <XYZ xyz, typename T>
std::enable_if_t<xyz == XYZ_31, T> cie_x(T wave)
{
        return xyz_implementation::x_31(wave);
}
template <XYZ xyz, typename T>
std::enable_if_t<xyz == XYZ_31, T> cie_y(T wave)
{
        return xyz_implementation::y_31(wave);
}
template <XYZ xyz, typename T>
std::enable_if_t<xyz == XYZ_31, T> cie_z(T wave)
{
        return xyz_implementation::z_31(wave);
}

template <XYZ xyz, typename T>
std::enable_if_t<xyz == XYZ_31, T> cie_x_integral(T wave_1, T wave_2)
{
        return xyz_implementation::x_31_integral(wave_1, wave_2);
}
template <XYZ xyz, typename T>
std::enable_if_t<xyz == XYZ_31, T> cie_y_integral(T wave_1, T wave_2)
{
        return xyz_implementation::y_31_integral(wave_1, wave_2);
}
template <XYZ xyz, typename T>
std::enable_if_t<xyz == XYZ_31, T> cie_z_integral(T wave_1, T wave_2)
{
        return xyz_implementation::z_31_integral(wave_1, wave_2);
}

template <XYZ xyz, typename T>
std::enable_if_t<xyz == XYZ_64, T> cie_x(T wave)
{
        return xyz_implementation::x_64(wave);
}
template <XYZ xyz, typename T>
std::enable_if_t<xyz == XYZ_64, T> cie_y(T wave)
{
        return xyz_implementation::y_64(wave);
}
template <XYZ xyz, typename T>
std::enable_if_t<xyz == XYZ_64, T> cie_z(T wave)
{
        return xyz_implementation::z_64(wave);
}
}
