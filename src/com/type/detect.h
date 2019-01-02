/*
Copyright (C) 2017-2019 Topological Manifold

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
#include <type_traits>
#include <vector>

namespace type_detect_implementation
{
struct IsArray
{
        template <typename T>
        struct S
        {
                static constexpr bool value = false;
        };
        template <typename T, size_t N>
        struct S<std::array<T, N>>
        {
                static constexpr bool value = true;
        };
};

struct IsVector
{
        template <typename T>
        struct S
        {
                static constexpr bool value = false;
        };
        template <typename... Args>
        struct S<std::vector<Args...>>
        {
                static constexpr bool value = true;
        };
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
template <typename Container>
class HasBeginEnd
{
        template <typename T>
        static decltype(std::begin(std::declval<T>()), std::end(std::declval<T>()), std::cbegin(std::declval<T>()),
                        std::cend(std::declval<T>()), std::true_type())
        f(int);
        template <typename>
        static std::false_type f(...);

public:
        static constexpr bool value = std::is_same_v<decltype(f<Container>(0)), std::true_type>;
};
#pragma GCC diagnostic pop
}

template <typename T>
inline constexpr bool is_array = type_detect_implementation::IsArray::S<std::remove_cv_t<T>>::value;
template <typename T>
inline constexpr bool is_vector = type_detect_implementation::IsVector::S<std::remove_cv_t<T>>::value;

template <typename T>
inline constexpr bool has_begin_end = type_detect_implementation::HasBeginEnd<T>::value;

static_assert(is_array<const std::array<int, 1>>);
static_assert(is_vector<const std::vector<int>>);

static_assert(!is_array<const std::vector<int>>);
static_assert(!is_vector<const std::array<int, 1>>);

static_assert(has_begin_end<const std::array<int, 1>>);
static_assert(has_begin_end<std::vector<double>&>);
static_assert(!has_begin_end<int>);
static_assert(!has_begin_end<double*>);
