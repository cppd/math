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

#include "error.h"

#include "type/concept.h"
#include "type/limit.h"

#include <algorithm>
#include <cstddef>

namespace ns
{
template <typename T>
void sort_and_unique(T* const v)
{
        std::sort(v->begin(), v->end());
        v->erase(std::unique(v->begin(), v->end()), v->end());
}

template <typename T>
        requires (!std::is_pointer_v<T>)
[[nodiscard]] T sort_and_unique(T v)
{
        sort_and_unique(&v);
        return v;
}

template <typename T>
[[nodiscard]] constexpr bool all_non_negative(const T& data)
{
        return std::all_of(
                data.cbegin(), data.cend(),
                [](const auto& v)
                {
                        return v >= 0;
                });
}

template <typename T>
[[nodiscard]] constexpr bool all_positive(const T& data)
{
        return std::all_of(
                data.cbegin(), data.cend(),
                [](const auto& v)
                {
                        return v > 0;
                });
}

template <typename T>
[[nodiscard]] constexpr bool all_negative(const T& data)
{
        return std::all_of(
                data.cbegin(), data.cend(),
                [](const auto& v)
                {
                        return v < 0;
                });
}

template <typename Result, typename Data>
[[nodiscard]] constexpr Result multiply_all(const Data& data)
{
        static_assert(
                (Integral<typename Data::value_type> && Integral<Result>)
                || (FloatingPoint<typename Data::value_type> && FloatingPoint<Result>));
        static_assert(Signed<typename Data::value_type> == Signed<Result>);
        static_assert(Limits<typename Data::value_type>::digits() <= Limits<Result>::digits());

        if (data.empty())
        {
                error("Empty container for multiply all");
        }

        Result res = data[0];
        for (std::size_t i = 1; i < data.size(); ++i)
        {
                res *= data[i];
        }

        return res;
}

template <typename Result, typename Data>
[[nodiscard]] constexpr Result add_all(const Data& data)
{
        static_assert(
                (Integral<typename Data::value_type> && Integral<Result>)
                || (FloatingPoint<typename Data::value_type> && FloatingPoint<Result>));
        static_assert(Signed<typename Data::value_type> == Signed<Result>);
        static_assert(Limits<typename Data::value_type>::digits() <= Limits<Result>::digits());

        if (data.empty())
        {
                error("Empty container for add all");
        }

        Result res = data[0];
        for (std::size_t i = 1; i < data.size(); ++i)
        {
                res += data[i];
        }

        return res;
}

// template <typename T1, typename T2>
// [[nodiscard]] bool intersect(T1 t1, T2 t2)
// {
//         std::sort(t1.begin(), t1.end());
//         std::sort(t2.begin(), t2.end());
//
//         auto i1 = std::cbegin(t1);
//         auto i2 = std::cbegin(t2);
//
//         while (true)
//         {
//                 if (i1 == std::cend(t1))
//                 {
//                         return false;
//                 }
//
//                 if (i2 == std::cend(t2))
//                 {
//                         return false;
//                 }
//
//                 if (*i1 < *i2)
//                 {
//                         ++i1;
//                 }
//                 else if (*i1 > *i2)
//                 {
//                         ++i2;
//                 }
//                 else if (!(*i1 == *i2))
//                 {
//                         error("Failed to find intersection. Not (a < b) and not (a > b) and not (a == b)");
//                 }
//                 else
//                 {
//                         return true;
//                 }
//         }
// }
}
