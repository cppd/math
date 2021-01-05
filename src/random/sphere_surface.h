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

#include <src/com/constant.h>
#include <src/com/error.h>
#include <src/numerical/integrate.h>

#include <cmath>
#include <numeric>

namespace ns::random
{
/*
 Определение отношения интегралов
 1. По поверхности сферы от константы 1.
 2. По поверхности сферы от косинуса угла между вектором из центра сферы
    к точке на сфере и одной из координатных осей.

 Assuming[n >= 2 && k >= 0,
  Integrate[Sin[x]^(n - 2), {x, 0, Pi/2}]/
   Integrate[(Sin[x]^(n - 2))*(Cos[x]^k), {x, 0, Pi/2}]]

 (Sqrt[Pi] Gamma[(k+n)/2])/(Gamma[(1+k)/2] Gamma[n/2])

   Используется обобщённое полярное преобразование.
 Якобиан J = r^(n-1) ⋅ sin(φ(1))^(n-2) ⋅ sin(φ(2))^(n-3) ⋅ ... ⋅ sin(φ(n-2)).
 Для единичной сферы надо найти отношение интегралов от функций

                  sin(φ(1))^(n-2) ⋅ sin(φ(2))^(n-3) ⋅ ... ⋅ sin(φ(n-2))
      cos(φ(1)) ⋅ sin(φ(1))^(n-2) ⋅ sin(φ(2))^(n-3) ⋅ ... ⋅ sin(φ(n-2)),

 где 0 ≤ φ(1) ≤ π/2, 0 ≤ φ(2)...φ(n-2) ≤ π, 0 ≤ φ(n-1) ≤ 2π.

   Различие только в переменной φ(1), поэтому надо найти отношение
 интегралов от функций
               sin(φ)^(n-2)
      cos(φ) ⋅ sin(φ)^(n-2),
 где 0 ≤ φ ≤ π/2.

   Интегралы от функции cos(φ)^a ⋅ sin(φ)^b на интервале 0 ≤ φ ≤ π/2 равны
      1/2 ⋅ beta((a+1)/2, (b+1)/2).
 Первый интеграл равен 1/2 ⋅ beta(1/2, (n-1)/2)
 Второй интеграл равен 1/2 ⋅ beta(1, (n-1)/2)
 Отношение равно

      beta(1/2, (n-1)/2) / beta(1, (n-1)/2).

   Записывая бета-функции через гамма-функции и используя свойства гамма-функций,
 получается
      sqrt(π)/2 ⋅ (n-1) ⋅ gamma((n-1)/2) / gamma(n/2).
 Далее вычисление значений гамма-функций с уменьшением значения аргумента.
  Если n чётное число, то sqrt(π) от gamma(1/2) будет в числителе, а в знаменателе
 будет на одну двойку больше
                                (n-3)/2 ⋅ (n-5)/2 ⋅ ...
 sqrt(π)/2 ⋅ (n-1) ⋅ sqrt(π) ⋅ -------------------------
                                (n-2)/2 ⋅ (n-4)/2 ⋅ ...

  π  (n-1) (n-3) ...   (int(n)/2 раз, включая умножение на единицу)
 -----------------------------------------
  2  (n-2) (n-4) ...   (int(n)/2 - 1 раз)

   Если нечётное, то sqrt(π) от gamma(1/2) будет в знаменателе, а количество двоек
 будет одинаковое в числителе и знаменателе
                        1       (n-3)/2 ⋅ (n-5)/2 ⋅ ...
 sqrt(π)/2 ⋅ (n-1) ⋅ ------- ⋅ -------------------------
                     sqrt(π)    (n-2)/2 ⋅ (n-4)/2 ⋅ ...

  (n-1) (n-3) ...   (int(n)/2 раз)
 ----------------------------------
  (n-2) (n-4) ...   (int(n)/2 раз, включая умножение на единицу)


 Фихтенгольц Г. М.
 Курс дифференциального и интегрального исчисления, 8-е изд.
 ФИЗМАТЛИТ, 2003

 Глава четырнадцатая
 ИНТЕГРАЛЫ, ЗАВИСЯЩИЕ ОТ ПАРАМЕТРА
 § 5. Эйлеровы интегралы
 534. Примеры

 Глава восемнадцатая
 ТРОЙНЫЕ И МНОГОКРАТНЫЕ ИНТЕГРАЛЫ
 § 5. Многократные интегралы
 676. Примеры
*/

constexpr long double cosine_sphere_coefficient(const unsigned N)
{
        ASSERT(N >= 2);

        unsigned long long divident = 1;
        unsigned long long divisor = (N & 1) == 0 ? 2 : 1;

        bool overflow = false;

        for (int i = N - 1; i > 1; i -= 2)
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
                p = (N & 1) == 0 ? 0.5 : 1;
                for (int i = N - 1; i > 1; i -= 2)
                {
                        p *= i;
                        if (i > 2)
                        {
                                p /= (i - 1);
                        }
                }
        }

        if ((N & 1) == 0)
        {
                p *= PI<long double>;
        }

        return p;
}

// 2 * pow(π, n/2) / gamma(n/2)
constexpr long double sphere_area(unsigned N)
{
        ASSERT(N >= 2);

        if ((N & 1) == 0)
        {
                int n = N / 2;
                // 2 * pow(π, n) / gamma(n)
                // gamma(n) = (n - 1)!
                long double res = 2;
                res *= PI<long double>;
                if (n >= 2)
                {
                        res *= PI<long double>;
                }
                for (int i = n - 1; i > 1; --i)
                {
                        res /= i;
                        res *= PI<long double>;
                }
                return res;
        }

        int n = N / 2;
        // 2 * pow(π, 1/2 + n) / gamma(1/2 + n)
        // 2 * pow(π, 1/2 + n) / pow(π, 1/2) / (2n!) * pow(4, n) * n!
        // 2 * pow(π, n) / (2n!) * pow(4, n) * n!
        long double res = 2;
        for (int i = 2 * n; i > n; --i)
        {
                res /= i;
                res *= 4;
                res *= PI<long double>;
        }
        return res;
}

template <unsigned N, typename T>
T sphere_relative_area(std::type_identity_t<T> a, std::type_identity_t<T> b)
{
        static_assert(std::is_floating_point_v<T>);
        static_assert(N >= 2);

        // Assuming[Element[n,Integers]&&n>=0,Integrate[Sin[x]^n,x]]
        // -Cos[x] Hypergeometric2F1[1/2,(1-n)/2,3/2,Cos[x]^2] Sin[x]^(1+n) (Sin[x]^2)^(1/2 (-1-n))
        // For[i=2,i<=10,++i,f=Integrate[Sin[x]^(i-2),{x, a, b}];Print[i];Print[f]]
        // For[i=2,i<=10,++i,f=Simplify[Integrate[Sin[x]^(i-2),x]];Print[i];Print[f]]

        if constexpr (N == 2)
        {
                return b - a;
        }
        if constexpr (N == 3)
        {
                return std::cos(a) - std::cos(b);
        }
        if constexpr (N == 4)
        {
                return (2 * b - 2 * a - std::sin(2 * b) + std::sin(2 * a)) / 4;
        }
        if constexpr (N == 5)
        {
                return (9 * std::cos(a) - std::cos(3 * a) - 9 * std::cos(b) + std::cos(3 * b)) / 12;
        }
        if constexpr (N == 6)
        {
                return (-12 * a + 12 * b + 8 * std::sin(2 * a) - std::sin(4 * a) - 8 * std::sin(2 * b)
                        + std::sin(4 * b))
                       / 32;
        }
        if constexpr (N == 7)
        {
                return (150 * std::cos(a) - 25 * std::cos(3 * a) + 3 * std::cos(5 * a) - 150 * std::cos(b)
                        + 25 * std::cos(3 * b) - 3 * std::cos(5 * b))
                       / 240;
        }
        if constexpr (N == 8)
        {
                return (-60 * a + 60 * b + 45 * std::sin(2 * a) - 9 * std::sin(4 * a) + std::sin(6 * a)
                        - 45 * std::sin(2 * b) + 9 * std::sin(4 * b) - std::sin(6 * b))
                       / 192;
        }
        if constexpr (N == 9)
        {
                return (1225 * std::cos(a) - 245 * std::cos(3 * a) + 49 * std::cos(5 * a) - 5 * std::cos(7 * a)
                        - 1225 * std::cos(b) + 245 * std::cos(3 * b) - 49 * std::cos(5 * b) + 5 * std::cos(7 * b))
                       / 2240;
        }
        if constexpr (N == 10)
        {
                return (-840 * a + 840 * b + 672 * std::sin(2 * a) - 168 * std::sin(4 * a) + 32 * std::sin(6 * a)
                        - 3 * std::sin(8 * a) - 672 * std::sin(2 * b) + 168 * std::sin(4 * b) - 32 * std::sin(6 * b)
                        + 3 * std::sin(8 * b))
                       / 3072;
        }
        if constexpr (N > 10)
        {
                return ns::numerical::integrate<T>(
                        [](T x)
                        {
                                return std::pow(std::sin(x), T(N - 2));
                        },
                        a, b, /*count*/ 100);
        }
}
}
