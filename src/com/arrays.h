/*
Copyright (C) 2017-2022 Topological Manifold

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
#include <utility>

namespace ns
{
namespace arrays_implementation
{
template <typename T, T... I>
constexpr std::array<T, sizeof...(I)> make_array_sequence(std::integer_sequence<T, I...>&&)
{
        return {I...};
}

template <typename T, std::size_t... I>
constexpr std::array<T, sizeof...(I)> make_array_value(std::integer_sequence<std::size_t, I...>&&, const T& v)
{
        return {(static_cast<void>(I), v)...};
}

template <std::size_t VALUE_INDEX, typename T, std::size_t... I>
constexpr std::array<T, sizeof...(I)> make_array_one_value(std::integer_sequence<std::size_t, I...>&&, const T& v)
{
        static_assert(VALUE_INDEX >= 0 && VALUE_INDEX < sizeof...(I));

        return {(I == VALUE_INDEX ? v : 0)...};
}

//

template <typename T, std::size_t... I>
constexpr std::array<T, sizeof...(I)> del_elem(
        const std::array<T, sizeof...(I) + 1>& a,
        const unsigned pos,
        std::integer_sequence<std::size_t, I...>&&)
{
        static_assert(((I < sizeof...(I)) && ...));

        return std::array<T, sizeof...(I)>{(I < pos ? a[I] : a[I + 1])...};
}
}

//

template <typename T, std::size_t N>
constexpr std::array<T, N> make_array_sequence()
{
        return arrays_implementation::make_array_sequence(std::make_integer_sequence<T, N>());
}

template <typename T, std::size_t N>
constexpr std::array<T, N> make_array_value(const T& v)
{
        return arrays_implementation::make_array_value(std::make_integer_sequence<std::size_t, N>(), v);
}

template <typename T, std::size_t N, std::size_t VALUE_INDEX>
constexpr std::array<T, N> make_array_one_value(const T& v)
{
        return arrays_implementation::make_array_one_value<VALUE_INDEX>(
                std::make_integer_sequence<std::size_t, N>(), v);
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
        static_assert(N > 1);
        ASSERT(pos < N);

        return arrays_implementation::del_elem<T>(a, pos, std::make_integer_sequence<std::size_t, N - 1>());
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
