/*
Copyright (C) 2017-2024 Topological Manifold

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
#include <src/com/exponent.h>
#include <src/numerical/integrate.h>

#include <cmath>
#include <numeric>
#include <optional>
#include <type_traits>

namespace ns::geometry::shapes
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

namespace sphere_integral_implementation
{
template <unsigned N>
constexpr std::optional<long double> integer_computation()
{
        static_assert(N >= 2);

        unsigned long long divident = 1;
        unsigned long long divisor = (N & 1) == 0 ? 2 : 1;

        for (int i = N - 1; i > 1; i -= 2)
        {
                const unsigned long long new_divident = divident * i;
                if (new_divident <= divident)
                {
                        return std::nullopt;
                }

                divident = new_divident;

                if (i > 2)
                {
                        const unsigned long long new_divisor = divisor * (i - 1);
                        if (new_divisor <= divisor)
                        {
                                return std::nullopt;
                        }

                        divisor = new_divisor;
                }

                const unsigned long long gcd = std::gcd(divident, divisor);
                if (gcd > 1)
                {
                        divident /= gcd;
                        divisor /= gcd;
                }
        }

        return static_cast<long double>(divident) / static_cast<long double>(divisor);
}

template <unsigned N>
constexpr long double floating_point_computation()
{
        static_assert(N >= 2);

        long double p = (N & 1) == 0 ? 0.5 : 1;
        for (int i = N - 1; i > 1; i -= 2)
        {
                p *= i;
                if (i > 2)
                {
                        p /= (i - 1);
                }
        }
        return p;
}
}

template <unsigned N, typename T>
inline constexpr T SPHERE_UNIT_INTEGRAL_OVER_COSINE_INTEGRAL = []
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        namespace impl = sphere_integral_implementation;

        long double p = 0;

        if (const auto value = impl::integer_computation<N>())
        {
                p = *value;
        }
        else
        {
                p = impl::floating_point_computation<N>();
        }

        if ((N & 1) == 0)
        {
                p *= PI<long double>;
        }

        return p;
}();

/*
Assuming[n>=2,Integrate[(Sin[x]^(n-2))*Cos[x],{x,0,Pi/2}]]
1 / (n - 1)
*/
template <unsigned N, typename T>
inline constexpr T SPHERE_INTEGRATE_COSINE_FACTOR_OVER_HEMISPHERE = []
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        if constexpr (N > 2)
        {
                return SPHERE_AREA<N - 1, long double> / (N - 1);
        }
        return 2.0L;
}();

/*
Assuming[n>=2,Integrate[(Sin[x]^(n-2))*Cos[x],x]]
pow(sin(x), n - 1) / (n - 1)
*/
template <unsigned N, typename T>
        requires (N >= 3)
T sphere_integrate_cosine_factor(const std::type_identity_t<T> a, const std::type_identity_t<T> b)
{
        static_assert(std::is_floating_point_v<T>);
        ASSERT(a >= 0 && a < b);
        if (a == 0)
        {
                return SPHERE_AREA<N - 1, T> * power<N - 1>(std::sin(b)) / (N - 1);
        }
        return SPHERE_AREA<N - 1, T> * (power<N - 1>(std::sin(b)) - power<N - 1>(std::sin(a))) / (N - 1);
}

template <unsigned N, typename T>
        requires (N == 2)
T sphere_integrate_cosine_factor(const std::type_identity_t<T> a, const std::type_identity_t<T> b)
{
        static_assert(std::is_floating_point_v<T>);
        ASSERT(a >= 0 && a < b);
        if (a == 0)
        {
                return 2 * std::sin(b);
        }
        return 2 * (std::sin(b) - std::sin(a));
}

/*
Assuming[n>=2,Integrate[(Sin[x]^(n-2))*Cos[x],{x,0,Pi/2}]]
1 / (n - 1)
*/
template <unsigned N, typename T, typename F>
T sphere_cosine_weighted_average_by_angle(const F& f, const int count)
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);
        static_assert(std::is_same_v<T, decltype(f(T()))>);

        const auto function = [&](const T v)
        {
                const T cosine = std::max(T{0}, std::cos(v));
                const T sine = std::sin(v);
                return power<N - 2>(sine) * cosine * f(v);
        };

        return (N - 1) * numerical::integrate(function, T{0}, PI<T> / 2, count);
}

template <unsigned N, typename T, typename F>
T sphere_cosine_weighted_average_by_cosine(const F& f, const int count)
{
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);
        static_assert(std::is_same_v<T, decltype(f(T()))>);

        const auto function = [&](const T v)
        {
                const T cosine = std::max(T{0}, std::cos(v));
                const T sine = std::sin(v);
                return power<N - 2>(sine) * cosine * f(cosine);
        };

        return (N - 1) * numerical::integrate(function, T{0}, PI<T> / 2, count);
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
        static_assert(N >= 2);
        static_assert(std::is_floating_point_v<T>);

        if constexpr (N == 3)
        {
                return power<1>(2 * PI<T>) / (1 + k);
        }
        else if constexpr (N == 5)
        {
                return power<2>(2 * PI<T>) / ((1 + k) * (3 + k));
        }
        else if constexpr (N == 7)
        {
                return power<3>(2 * PI<T>) / ((1 + k) * (3 + k) * (5 + k));
        }
        else
        {
                return std::pow(PI<T>, T{N - 1} / 2) * std::exp(std::lgamma((k + 1) / 2) - std::lgamma((k + N) / 2));
        }
}
}
