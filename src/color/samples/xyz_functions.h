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
Chris Wyman, Peter-Pike Sloan, Peter Shirley.
Simple Analytic Approximations to the CIE XYZ Color Matching Functions.
Journal of Computer Graphics Techniques, Vol. 2, No. 2, 2013.
*/

#pragma once

#include <src/com/error.h>

#include <cmath>
#include <type_traits>

namespace ns::color::samples
{
namespace xyz_functions_implementation
{
template <typename T>
T g(const T wave, const std::type_identity_t<T> m, const std::type_identity_t<T> t1, const std::type_identity_t<T> t2)
{
        const T v = (wave - m) * (wave < m ? t1 : t2);
        return std::exp(T{-0.5} * v * v);
}

// Integrate[Exp[-1/2*(t*(x-m))^2],{x,a,b}]
template <typename T>
T g_integral(const T wave_1, const T wave_2, const std::type_identity_t<T> m, const std::type_identity_t<T> t)
{
        // sqrt(2)
        static constexpr T SQRT_2 = 1.4142135623730950488016887242096980785696718753769L;
        // sqrt(PI/2)
        static constexpr T SQRT_PI_2 = 1.2533141373155002512078826424055226265034933703050L;

        const T ts = t / SQRT_2;
        return (SQRT_PI_2 / t) * (std::erf(ts * (wave_2 - m)) - std::erf(ts * (wave_1 - m)));
}

template <typename T>
T g_integral(
        const T wave_1,
        const T wave_2,
        const std::type_identity_t<T> m,
        const std::type_identity_t<T> t1,
        const std::type_identity_t<T> t2)
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
}

//

template <typename T>
T cie_x_31(const T wave)
{
        static_assert(std::is_floating_point_v<T>);
        namespace impl = xyz_functions_implementation;

        const T g1 = impl::g(wave, 442.0, 0.0624, 0.0374);
        const T g2 = impl::g(wave, 599.8, 0.0264, 0.0323);
        const T g3 = impl::g(wave, 501.1, 0.0490, 0.0382);
        return T{0.362} * g1 + T{1.056} * g2 + T{-0.065} * g3;
}

template <typename T>
T cie_x_31_integral(const T wave_1, const T wave_2)
{
        static_assert(std::is_floating_point_v<T>);
        namespace impl = xyz_functions_implementation;

        const T g1 = impl::g_integral(wave_1, wave_2, 442.0, 0.0624, 0.0374);
        const T g2 = impl::g_integral(wave_1, wave_2, 599.8, 0.0264, 0.0323);
        const T g3 = impl::g_integral(wave_1, wave_2, 501.1, 0.0490, 0.0382);
        return T{0.362} * g1 + T{1.056} * g2 + T{-0.065} * g3;
}

//

template <typename T>
T cie_y_31(const T wave)
{
        static_assert(std::is_floating_point_v<T>);
        namespace impl = xyz_functions_implementation;

        const T g1 = impl::g(wave, 568.8, 0.0213, 0.0247);
        const T g2 = impl::g(wave, 530.9, 0.0613, 0.0322);
        return T{0.821} * g1 + T{0.286} * g2;
}

template <typename T>
T cie_y_31_integral(const T wave_1, const T wave_2)
{
        static_assert(std::is_floating_point_v<T>);
        namespace impl = xyz_functions_implementation;

        const T g1 = impl::g_integral(wave_1, wave_2, 568.8, 0.0213, 0.0247);
        const T g2 = impl::g_integral(wave_1, wave_2, 530.9, 0.0613, 0.0322);
        return T{0.821} * g1 + T{0.286} * g2;
}

//

template <typename T>
T cie_z_31(const T wave)
{
        static_assert(std::is_floating_point_v<T>);
        namespace impl = xyz_functions_implementation;

        const T g1 = impl::g(wave, 437.0, 0.0845, 0.0278);
        const T g2 = impl::g(wave, 459.0, 0.0385, 0.0725);
        return T{1.217} * g1 + T{0.681} * g2;
}

template <typename T>
T cie_z_31_integral(const T wave_1, const T wave_2)
{
        static_assert(std::is_floating_point_v<T>);
        namespace impl = xyz_functions_implementation;

        const T g1 = impl::g_integral(wave_1, wave_2, 437.0, 0.0845, 0.0278);
        const T g2 = impl::g_integral(wave_1, wave_2, 459.0, 0.0385, 0.0725);
        return T{1.217} * g1 + T{0.681} * g2;
}

//

template <typename T>
T cie_x_64(const T wave)
{
        static_assert(std::is_floating_point_v<T>);

        const T t1 = std::log((wave + T{570.1}) / T{1014});
        const T t2 = std::log((T{1338} - wave) / T{743.5});
        return T{0.398} * std::exp(T{-1250} * t1 * t1) + T{1.132} * std::exp(T{-234} * t2 * t2);
}

/*
Integrate[Exp[t1*Log[(x+m)/t2]^2],x]
(Sqrt[Pi]*t2*Erfi[(1 + 2*t1*Log[(m + x)/t2])/(2*Sqrt[t1])])/(E^(1/4/t1)*(2*Sqrt[t1]))

Integrate[Exp[t1*Log[(m-x)/t2]^2],x]
-((Sqrt[Pi]*t2*Erfi[(1 + 2*t1*Log[(m - x)/t2])/(2*Sqrt[t1])])/(E^(1/4/t1)*(2*Sqrt[t1])))

Simplify[Integrate[0.398`30*Exp[-1250*Log[(x+570.1`30)/1014]^2],x]]
Simplify[Integrate[1.132`30*Exp[-234*Log[(1338 - x)/743.5`30]^2], x]]
*/
template <typename T>
T cie_x_64_integral(const T wave_1, const T wave_2)
{
        static_assert(std::is_floating_point_v<T>);

        const auto erf_1 = [](T w)
        {
                return std::erf(
                        T{244.731714089055124500775830124L}
                        - T{35.3553390593273762200422181052L} * std::log(T{570.1} + w));
        };

        const auto erf_2 = [](T w)
        {
                return std::erf(
                        T{101.167181069172083793364304205L}
                        - T{15.297058540778354490084672327L} * std::log(T{1338} - w));
        };

        const T s1 = T{-10.1180732728004060626662605914L} * (erf_1(wave_2) - erf_1(wave_1));
        const T s2 = T{48.812202187820072511214075419L} * (erf_2(wave_2) - erf_2(wave_1));

        return s1 + s2;
}

//

template <typename T>
T cie_y_64(const T wave)
{
        static_assert(std::is_floating_point_v<T>);

        const T t = (wave - T{556.1}) / T{46.14};
        return T{1.011} * std::exp(T{-0.5} * t * t);
}

/*
Integrate[Exp[-1/2*((x-m)/t)^2],x]
(-Sqrt[Pi/2])*t*Erf[(m - x)/(Sqrt[2]*t)]

Integrate[1.011`30*Exp[-1/2*((x-556.1`30)/46.14`30)^2],x]
*/
template <typename T>
T cie_y_64_integral(const T wave_1, const T wave_2)
{
        static_assert(std::is_floating_point_v<T>);

        const auto erf = [](const T w)
        {
                return std::erf(T{0.0153252444990582471695024785892L} * (w - T{556.1}));
        };

        return T{58.464021352990290588229753877L} * (erf(wave_2) - erf(wave_1));
}

//

template <typename T>
T cie_z_64(const T wave)
{
        static_assert(std::is_floating_point_v<T>);

        const T t = std::log((wave - T{265.8}) / T{180.4});
        return T{2.060} * std::exp(T{-32} * t * t);
}

/*
Integrate[Exp[t1*Log[(x-m)/t2]^2],x]
(Sqrt[Pi]*t2*Erfi[(1 + 2*t1*Log[(-m + x)/t2])/(2*Sqrt[t1])])/(E^(1/4/t1)*(2*Sqrt[t1]))

Simplify[Integrate[2.06`30*Exp[-32*Log[(x-265.8`30)/180.4`30]^2],x]]
*/
template <typename T>
T cie_z_64_integral(const T wave_1, const T wave_2)
{
        static_assert(std::is_floating_point_v<T>);

        const auto erf = [](const T w)
        {
                return std::erf(
                        T{29.4767452173751382583403029225L}
                        - T{5.6568542494923801952067548968L} * std::log(w - T{265.8}));
        };

        return T{-58.676828321407216171251716717L} * (erf(wave_2) - erf(wave_1));
}
}
