/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <src/com/constant.h>
#include <src/numerical/integrate.h>

#include <cmath>
#include <type_traits>

namespace ns::geometry::shapes
{
template <unsigned N, typename T>
inline constexpr T SPHERE_AREA = []
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        constexpr int M = N / 2;

        // 2 * pow(π, n/2) / gamma(n/2)

        if ((N & 1) == 0)
        {
                // 2 * pow(π, n) / gamma(n)
                // gamma(n) = (n - 1)!
                long double res = 2;
                res *= PI<long double>;
                if (M >= 2)
                {
                        res *= PI<long double>;
                }
                for (int i = M - 1; i > 1; --i)
                {
                        res /= i;
                        res *= PI<long double>;
                }
                return res;
        }

        // 2 * pow(π, 1/2 + n) / gamma(1/2 + n)
        // 2 * pow(π, 1/2 + n) / pow(π, 1/2) / (2n!) * pow(4, n) * n!
        // 2 * pow(π, n) / (2n!) * pow(4, n) * n!
        long double res = 2;
        for (int i = 2 * M; i > M; --i)
        {
                res /= i;
                res *= 4;
                res *= PI<long double>;
        }
        return res;
}();

template <unsigned N, typename T>
T sphere_relative_area(const std::type_identity_t<T> a, const std::type_identity_t<T> b)
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        // Assuming[Element[n,Integers]&&n>=0,Integrate[Sin[x]^n,x]]
        // -Cos[x] Hypergeometric2F1[1/2,(1-n)/2,3/2,Cos[x]^2] Sin[x]^(1+n) (Sin[x]^2)^(1/2 (-1-n))
        // For[i=2,i<=10,++i,f=Integrate[Sin[x]^(i-2),{x, a, b}];Print[i];Print[f]]
        // For[i=2,i<=10,++i,f=Simplify[Integrate[Sin[x]^(i-2),x]];Print[i];Print[f]]

        if constexpr (N == 2)
        {
                return b - a;
        }
        else if constexpr (N == 3)
        {
                return std::cos(a) - std::cos(b);
        }
        else if constexpr (N == 4)
        {
                return (2 * b - 2 * a - std::sin(2 * b) + std::sin(2 * a)) / 4;
        }
        else if constexpr (N == 5)
        {
                return (9 * std::cos(a) - std::cos(3 * a) - 9 * std::cos(b) + std::cos(3 * b)) / 12;
        }
        else if constexpr (N == 6)
        {
                return (-12 * a + 12 * b + 8 * std::sin(2 * a) - std::sin(4 * a) - 8 * std::sin(2 * b)
                        + std::sin(4 * b))
                       / 32;
        }
        else if constexpr (N == 7)
        {
                return (150 * std::cos(a) - 25 * std::cos(3 * a) + 3 * std::cos(5 * a) - 150 * std::cos(b)
                        + 25 * std::cos(3 * b) - 3 * std::cos(5 * b))
                       / 240;
        }
        else if constexpr (N == 8)
        {
                return (-60 * a + 60 * b + 45 * std::sin(2 * a) - 9 * std::sin(4 * a) + std::sin(6 * a)
                        - 45 * std::sin(2 * b) + 9 * std::sin(4 * b) - std::sin(6 * b))
                       / 192;
        }
        else if constexpr (N == 9)
        {
                return (1225 * std::cos(a) - 245 * std::cos(3 * a) + 49 * std::cos(5 * a) - 5 * std::cos(7 * a)
                        - 1225 * std::cos(b) + 245 * std::cos(3 * b) - 49 * std::cos(5 * b) + 5 * std::cos(7 * b))
                       / 2240;
        }
        else if constexpr (N == 10)
        {
                return (-840 * a + 840 * b + 672 * std::sin(2 * a) - 168 * std::sin(4 * a) + 32 * std::sin(6 * a)
                        - 3 * std::sin(8 * a) - 672 * std::sin(2 * b) + 168 * std::sin(4 * b) - 32 * std::sin(6 * b)
                        + 3 * std::sin(8 * b))
                       / 3072;
        }
        else
        {
                return ns::numerical::integrate<T>(
                        [](const T x)
                        {
                                return std::pow(std::sin(x), static_cast<T>(N - 2));
                        },
                        a, b, /*count*/ 100);
        }
}
}
