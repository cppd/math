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

#include <array>
#include <vector>

namespace ns
{
namespace type_detect_implementation
{
template <typename>
struct IsArray final : std::false_type
{
};
template <typename T, std::size_t N>
struct IsArray<std::array<T, N>> final : std::true_type
{
};

template <typename>
struct IsVector final : std::false_type
{
};
template <typename... Args>
struct IsVector<std::vector<Args...>> final : std::true_type
{
};
}

template <typename T>
inline constexpr bool is_array = type_detect_implementation::IsArray<std::remove_cv_t<T>>::value;
template <typename T>
inline constexpr bool is_vector = type_detect_implementation::IsVector<std::remove_cv_t<T>>::value;

template <typename T>
inline constexpr bool has_cbegin_cend = requires(const T& v)
{
        std::begin(v);
        std::end(v);
};
template <typename T>
inline constexpr bool has_data_and_size = requires(const T& v)
{
        std::data(v);
        std::size(v);
};
}
