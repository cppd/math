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

#include "sphere_area.h"

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/com/math.h>

#include <cmath>
#include <numeric>

namespace ns::geometry
{
/*
Assuming[n >= 2 && k >= 0,
  Integrate[Sin[x]^(n - 2), {x, 0, Pi/2}]/
    Integrate[(Sin[x]^(n - 2))*(Cos[x]^k), {x, 0, Pi/2}]]
sqrt(π) * gamma((k+n)/2) / (gamma((1+k)/2) * gamma(n/2))

for k = 1
sqrt(π) * gamma((n+1)/2) / gamma(n/2)

gamma(n+1) = n⋅gamma(n)
gamma(1/2) = sqrt(π)

if n is even
                    (n-1)/2 ⋅ (n-3)/2 ⋅ ...
sqrt(π) ⋅ sqrt(π) ⋅ -----------------------
                    (n-2)/2 ⋅ (n-4)/2 ⋅ ...
 π  (n-1) (n-3) ...
-------------------
 2  (n-2) (n-4) ...

if n is odd
              1      (n-1)/2 ⋅ (n-3)/2 ⋅ ...
sqrt(π) ⋅  ------- ⋅ -----------------------
           sqrt(π)   (n-2)/2 ⋅ (n-4)/2 ⋅ ...
 (n-1) (n-3) ...
----------------
 (n-2) (n-4) ...
*/
constexpr long double sphere_unit_integral_over_cosine_integral(const unsigned n)
{
        ASSERT(n >= 2);

        unsigned long long divident = 1;
        unsigned long long divisor = (n & 1) == 0 ? 2 : 1;

        bool overflow = false;

        for (int i = n - 1; i > 1; i -= 2)
        {
                unsigned long long new_divident = divident * i;
                if (new_divident <= divident)
                {
                        overflow = true;
                        break;
                }

                divident = new_divident;

                if (i > 2)
                {
                        unsigned long long new_divisor = divisor * (i - 1);
                        if (new_divisor <= divisor)
                        {
                                overflow = true;
                                break;
                        }

                        divisor = new_divisor;
                }

                unsigned long long gcd = std::gcd(divident, divisor);
                if (gcd > 1)
                {
                        divident /= gcd;
                        divisor /= gcd;
                }
        }

        long double p = 0;

        if (!overflow)
        {
                p = static_cast<long double>(divident) / static_cast<long double>(divisor);
        }
        else
        {
                p = (n & 1) == 0 ? 0.5 : 1;
                for (int i = n - 1; i > 1; i -= 2)
                {
                        p *= i;
                        if (i > 2)
                        {
                                p /= (i - 1);
                        }
                }
        }

        if ((n & 1) == 0)
        {
                p *= PI<long double>;
        }

        return p;
}

template <std::size_t N>
constexpr long double sphere_integrate_cosine_factor_over_hemisphere()
{
        return sphere_area<N>() / sphere_unit_integral_over_cosine_integral(N) / 2;
}

/*
hemisphereArea[n_]:=Power[\[Pi],n/2]/Gamma[n/2];
unitIntegral[n_]:=Integrate[Sin[x]^(n-2),{x,0,Pi/2}];
cosineIntegral[n_,k_]:=Integrate[(Sin[x]^(n-2))*(Cos[x]^k),{x,0,Pi/2}];
Assuming[Element[n,Integers]&&n>=2&&k>=1,hemisphereArea[n]*(cosineIntegral[n,k]/unitIntegral[n])]
(pow(π,(n - 1) / 2) * gamma((k + 1) / 2)) / gamma((k + n) / 2)
For[n=2,n<=11,++n,f=Assuming[k>=1,
  hemisphereArea[n]*(cosineIntegral[n,k]/unitIntegral[n])];Print[n];Print[f]]
*/
template <unsigned N, typename T>
T sphere_integrate_power_cosine_factor_over_hemisphere(const std::type_identity_t<T> k)
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

        if constexpr (N == 3)
        {
                return power<1>(2 * PI<T>) / (1 + k);
        }
        if constexpr (N == 5)
        {
                return power<2>(2 * PI<T>) / ((1 + k) * (3 + k));
        }
        if constexpr (N == 7)
        {
                return power<3>(2 * PI<T>) / ((1 + k) * (3 + k) * (5 + k));
        }
        return std::pow(PI<T>, (N - 1) / T(2)) * std::exp(std::lgamma((k + 1) / 2) - std::lgamma((k + N) / 2));
}
}
