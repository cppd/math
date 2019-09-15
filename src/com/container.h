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

#include "com/type/detect.h"

#include <iterator>
#include <utility>

template <typename T>
constexpr auto storage_size(const T& c) -> decltype(std::size(c))
{
        return std::size(c) * sizeof(typename T::value_type); // sizeof(c[0])
}

template <class T, size_t N>
constexpr size_t storage_size(const T (&)[N])
{
        return N * sizeof(T);
}

template <typename T>
std::enable_if_t<has_data_and_size<T> && has_begin_end<T>, size_t> data_size(const T& data)
{
        static_assert(!std::is_pointer_v<T>);
        return storage_size(data);
}

template <typename T>
std::enable_if_t<!has_data_and_size<T> && !has_begin_end<T>, size_t> data_size(const T&)
{
        static_assert(!std::is_pointer_v<T>);
        return sizeof(T);
}

template <typename T>
std::enable_if_t<has_data_and_size<T> && has_begin_end<T>, std::conditional_t<std::is_const_v<T>, const void*, void*>>
data_pointer(T& data)
{
        static_assert(!std::is_pointer_v<T>);
        return data.data();
}

template <typename T>
std::enable_if_t<!has_data_and_size<T> && !has_begin_end<T>, std::conditional_t<std::is_const_v<T>, const void*, void*>>
data_pointer(T& data)
{
        static_assert(!std::is_pointer_v<T>);
        return &data;
}
