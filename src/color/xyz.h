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

#include <cmath>

namespace ns::color
{
namespace xyz_implementation
{
template <typename T>
T g(T wave_in_nanometers, T m, T t1, T t2)
{
        T v = (wave_in_nanometers - m) * (wave_in_nanometers < m ? t1 : t2);
        return std::exp(T(-0.5) * v * v);
}
}

template <typename T>
T x_31(T wave_in_nanometers)
{
        static_assert(std::is_floating_point_v<T>);

        namespace impl = xyz_implementation;

        T g1 = impl::g(wave_in_nanometers, T(442.0), T(0.0624), T(0.0374));
        T g2 = impl::g(wave_in_nanometers, T(599.8), T(0.0264), T(0.0323));
        T g3 = impl::g(wave_in_nanometers, T(501.1), T(0.0490), T(0.0382));
        return T(0.362) * g1 + T(1.056) * g2 + T(-0.065) * g3;
}

template <typename T>
T y_31(T wave_in_nanometers)
{
        static_assert(std::is_floating_point_v<T>);

        namespace impl = xyz_implementation;

        T g1 = impl::g(wave_in_nanometers, T(568.8), T(0.0213), T(0.0247));
        T g2 = impl::g(wave_in_nanometers, T(530.9), T(0.0613), T(0.0322));
        return T(0.821) * g1 + T(0.286) * g2;
}

template <typename T>
T z_31(T wave_in_nanometers)
{
        static_assert(std::is_floating_point_v<T>);

        namespace impl = xyz_implementation;

        T g1 = impl::g(wave_in_nanometers, T(437.0), T(0.0845), T(0.0278));
        T g2 = impl::g(wave_in_nanometers, T(459.0), T(0.0385), T(0.0725));
        return T(1.217) * g1 + T(0.681) * g2;
}
}
