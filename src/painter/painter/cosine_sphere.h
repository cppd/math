/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <array>
#include <numeric>

namespace painter
{
/*
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

//
// Определение отношения интегралов
// 1. По поверхности сферы от константы 1.
// 2. По поверхности сферы от косинуса угла между вектором из центра сферы
//    к точке на сфере и одной из координатных осей.
//
//   Используется обобщённое полярное преобразование.
// Якобиан J = r^(n-1) ⋅ sin(φ(1))^(n-2) ⋅ sin(φ(2))^(n-3) ⋅ ... ⋅ sin(φ(n-2)).
// Для единичной сферы надо найти отношение интегралов от функций
//
//                  sin(φ(1))^(n-2) ⋅ sin(φ(2))^(n-3) ⋅ ... ⋅ sin(φ(n-2))
//      cos(φ(1)) ⋅ sin(φ(1))^(n-2) ⋅ sin(φ(2))^(n-3) ⋅ ... ⋅ sin(φ(n-2)),
//
// где 0 ≤ φ(1) ≤ π/2, 0 ≤ φ(2)...φ(n-2) ≤ π, 0 ≤ φ(n-1) ≤ 2π.
//
//   Различие только в переменной φ(1), поэтому надо найти отношение
// интегралов от функций
//               sin(φ)^(n-2)
//      cos(φ) ⋅ sin(φ)^(n-2),
// где 0 ≤ φ ≤ π/2.
//
//   Интегралы от функции cos(φ)^a ⋅ sin(φ)^b на интервале 0 ≤ φ ≤ π/2 равны
//      1/2 ⋅ beta((a+1)/2, (b+1)/2).
// Первый интеграл равен 1/2 ⋅ beta(1/2, (n-1)/2)
// Второй интеграл равен 1/2 ⋅ beta(1, (n-1)/2)
// Отношение равно
//
//      beta(1/2, (n-1)/2) / beta(1, (n-1)/2).
//
//   Записывая бета-функции через гамма-функции и используя свойства гамма-функций,
// получается
//      sqrt(π)/2 ⋅ (n-1) ⋅ gamma((n-1)/2) / gamma(n/2).
// Далее вычисление значений гамма-функций с уменьшением значения аргумента.
//  Если n чётное число, то sqrt(π) от gamma(1/2) будет в числителе, а в знаменателе
// будет на одну двойку больше
//                                (n-3)/2 ⋅ (n-5)/2 ⋅ ...
// sqrt(π)/2 ⋅ (n-1) ⋅ sqrt(π) ⋅ -------------------------
//                                (n-2)/2 ⋅ (n-4)/2 ⋅ ...
//
//  π  (n-1) (n-3) ...   (int(n)/2 раз, включая умножение на единицу)
// -----------------------------------------
//  2  (n-2) (n-4) ...   (int(n)/2 - 1 раз)
//
//   Если нечётное, то sqrt(π) от gamma(1/2) будет в знаменателе, а количество двоек
// будет одинаковое в числителе и знаменателе
//                        1       (n-3)/2 ⋅ (n-5)/2 ⋅ ...
// sqrt(π)/2 ⋅ (n-1) ⋅ ------- ⋅ -------------------------
//                     sqrt(π)    (n-2)/2 ⋅ (n-4)/2 ⋅ ...
//
//  (n-1) (n-3) ...   (int(n)/2 раз)
// ----------------------------------
//  (n-2) (n-4) ...   (int(n)/2 раз, включая умножение на единицу)
//
// Функция предназначена для использования только в константах constexpr,
// поэтому не нужно быстродействие вроде таблиц готовых значений и прочего.
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
}
