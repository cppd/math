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

#include "vector.h"

#include <array>
#include <utility>

namespace ns::numerical
{
namespace identity_implementation
{
template <std::size_t VALUE_INDEX, typename T, int... I>
constexpr Vector<sizeof...(I), T> make_vector_one_value(std::integer_sequence<int, I...>&&, const T& value)
{
        static_assert(VALUE_INDEX >= 0 && VALUE_INDEX < sizeof...(I));

        return Vector<sizeof...(I), T>((I == VALUE_INDEX ? value : 0)...);
}

template <std::size_t N, typename T, std::size_t VALUE_INDEX>
constexpr Vector<N, T> make_vector_one_value(const T& value)
{
        return make_vector_one_value<VALUE_INDEX>(std::make_integer_sequence<int, N>(), value);
}

template <typename T, int... I>
constexpr std::array<Vector<sizeof...(I), T>, sizeof...(I)> make_array_of_vector_one_value(
        std::integer_sequence<int, I...>&&,
        const T& value)
{
        return {make_vector_one_value<sizeof...(I), T, I>(value)...};
}

template <std::size_t N, typename T>
constexpr std::array<Vector<N, T>, N> make_array_of_vector_one_value(const T& value)
{
        return make_array_of_vector_one_value(std::make_integer_sequence<int, N>(), value);
}
}

template <std::size_t N, typename T>
inline constexpr std::array<Vector<N, T>, N> IDENTITY_ARRAY =
        identity_implementation::make_array_of_vector_one_value<N, T>(1);
}
