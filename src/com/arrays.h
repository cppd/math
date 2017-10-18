/*
Copyright (C) 2017 Topological Manifold

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

#include <array>
#include <utility>

namespace ArraysImplementation
{
template <typename T, T... I>
constexpr std::array<T, sizeof...(I)> make_array_sequence_impl(std::integer_sequence<T, I...>)
{
        return {{I...}};
}

template <typename T, size_t... I>
constexpr std::array<T, sizeof...(I)> make_array_value_impl(std::integer_sequence<size_t, I...>, const T& v)
{
        return {{(static_cast<void>(I), v)...}};
}
}

template <typename T, size_t N>
constexpr std::array<T, N> make_array_sequence()
{
        return ArraysImplementation::make_array_sequence_impl(std::make_integer_sequence<T, N>());
}

template <typename T, size_t N>
constexpr std::array<T, N> make_array_value(const T& v)
{
        return ArraysImplementation::make_array_value_impl(std::make_integer_sequence<size_t, N>(), v);
}
