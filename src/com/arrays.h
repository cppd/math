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

#include "error.h"

#include <array>
#include <cstddef>
#include <utility>

namespace ns
{
template <typename T, std::size_t N>
constexpr std::array<T, N> make_array_sequence()
{
        return []<T... I>(std::integer_sequence<T, I...>&&)
        {
                static_assert(sizeof...(I) == N);
                return std::array<T, N>{I...};
        }(std::make_integer_sequence<T, N>());
}

template <typename T, std::size_t N>
constexpr std::array<T, N> make_array_value(const T& v)
{
        return [&]<std::size_t... I>(std::integer_sequence<std::size_t, I...>&&)
        {
                static_assert(sizeof...(I) == N);
                return std::array<T, N>{(static_cast<void>(I), v)...};
        }(std::make_integer_sequence<std::size_t, N>());
}

template <std::size_t N>
inline constexpr std::array<unsigned char, N> SEQUENCE_UCHAR_ARRAY = make_array_sequence<unsigned char, N>();

//

template <typename T, std::size_t N>
constexpr std::array<T, N> set_elem(const std::array<T, N>& a, const unsigned pos, const T& v)
{
        static_assert(N >= 1);
        ASSERT(pos < N);
        std::array<T, N> res(a);
        res[pos] = v;
        return res;
}

template <typename T, std::size_t N>
constexpr std::array<T, N - 1> del_elem(const std::array<T, N>& a, const unsigned pos)
{
        static_assert(N >= 5);
        ASSERT(pos < N);
        return [&]<unsigned... I>(std::integer_sequence<unsigned, I...>&&)
        {
                static_assert(sizeof...(I) == N - 1);
                static_assert(((I < N - 1) && ...));
                return std::array<T, N - 1>{(I < pos ? a[I] : a[I + 1])...};
        }(std::make_integer_sequence<unsigned, N - 1>());
}

template <typename T>
constexpr std::array<T, 1> del_elem(const std::array<T, 2>& a, const unsigned pos)
{
        switch (pos)
        {
        case 0:
                return {a[1]};
        case 1:
                return {a[0]};
        default:
                error("pos > 1");
        }
}

template <typename T>
constexpr std::array<T, 2> del_elem(const std::array<T, 3>& a, const unsigned pos)
{
        switch (pos)
        {
        case 0:
                return {a[1], a[2]};
        case 1:
                return {a[0], a[2]};
        case 2:
                return {a[0], a[1]};
        default:
                error("pos > 2");
        }
}

template <typename T>
constexpr std::array<T, 3> del_elem(const std::array<T, 4>& a, const unsigned pos)
{
        switch (pos)
        {
        case 0:
                return {a[1], a[2], a[3]};
        case 1:
                return {a[0], a[2], a[3]};
        case 2:
                return {a[0], a[1], a[3]};
        case 3:
                return {a[0], a[1], a[2]};
        default:
                error("pos > 3");
        }
}

template <typename T>
constexpr std::array<T, 4> del_elem(const std::array<T, 5>& a, const unsigned pos)
{
        switch (pos)
        {
        case 0:
                return {a[1], a[2], a[3], a[4]};
        case 1:
                return {a[0], a[2], a[3], a[4]};
        case 2:
                return {a[0], a[1], a[3], a[4]};
        case 3:
                return {a[0], a[1], a[2], a[4]};
        case 4:
                return {a[0], a[1], a[2], a[3]};
        default:
                error("pos > 4");
        }
}
}
