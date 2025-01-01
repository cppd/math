/*
Copyright (C) 2017-2025 Topological Manifold

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

#include <algorithm>
#include <type_traits>

namespace ns
{
namespace sort_implementation
{
template <typename T>
void swap_sort(T& a, T& b)
{
        if (b < a)
        {
                std::swap(a, b);
        }
}
}

template <typename T>
        requires (std::tuple_size_v<std::remove_reference_t<T>> >= 6)
T&& sort(T&& v)
{
        std::sort(v.begin(), v.end());

        return std::forward<T>(v);
}

template <typename T>
        requires (std::tuple_size_v<std::remove_reference_t<T>> == 1)
T&& sort(T&& v)
{
        return std::forward<T>(v);
}

template <typename T>
        requires (std::tuple_size_v<std::remove_reference_t<T>> == 2)
T&& sort(T&& v)
{
        namespace impl = sort_implementation;

        impl::swap_sort(v[0], v[1]);

        return std::forward<T>(v);
}

template <typename T>
        requires (std::tuple_size_v<std::remove_reference_t<T>> == 3)
T&& sort(T&& v)
{
        namespace impl = sort_implementation;

        impl::swap_sort(v[0], v[1]);
        impl::swap_sort(v[0], v[2]);
        impl::swap_sort(v[1], v[2]);

        return std::forward<T>(v);
}

template <typename T>
        requires (std::tuple_size_v<std::remove_reference_t<T>> == 4)
T&& sort(T&& v)
{
        namespace impl = sort_implementation;

        impl::swap_sort(v[0], v[1]);
        impl::swap_sort(v[2], v[3]);
        impl::swap_sort(v[0], v[2]);
        impl::swap_sort(v[1], v[3]);
        impl::swap_sort(v[1], v[2]);

        return std::forward<T>(v);
}

template <typename T>
        requires (std::tuple_size_v<std::remove_reference_t<T>> == 5)
T&& sort(T&& v)
{
        namespace impl = sort_implementation;

        impl::swap_sort(v[0], v[1]);
        impl::swap_sort(v[2], v[3]);
        impl::swap_sort(v[1], v[3]);
        impl::swap_sort(v[3], v[4]);
        impl::swap_sort(v[0], v[1]);
        impl::swap_sort(v[2], v[3]);
        impl::swap_sort(v[0], v[2]);
        impl::swap_sort(v[1], v[3]);
        impl::swap_sort(v[1], v[2]);

        return std::forward<T>(v);
}
}
