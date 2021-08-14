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

#include <algorithm>
#include <array>

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

template <typename T, std::size_t N>
std::array<T, N>&& sort(std::array<T, N>&& v)
{
        static_assert(N >= 6);

        std::sort(v.begin(), v.end());

        return std::move(v);
}

template <typename T>
std::array<T, 1>&& sort(std::array<T, 1>&& v)
{
        return std::move(v);
}

template <typename T>
std::array<T, 2>&& sort(std::array<T, 2>&& v)
{
        namespace impl = sort_implementation;

        impl::swap_sort(v[0], v[1]);

        return std::move(v);
}

template <typename T>
std::array<T, 3>&& sort(std::array<T, 3>&& v)
{
        namespace impl = sort_implementation;

        impl::swap_sort(v[0], v[1]);
        impl::swap_sort(v[0], v[2]);
        impl::swap_sort(v[1], v[2]);

        return std::move(v);
}

template <typename T>
std::array<T, 4>&& sort(std::array<T, 4>&& v)
{
        namespace impl = sort_implementation;

        impl::swap_sort(v[0], v[1]);
        impl::swap_sort(v[2], v[3]);
        impl::swap_sort(v[0], v[2]);
        impl::swap_sort(v[1], v[3]);
        impl::swap_sort(v[1], v[2]);

        return std::move(v);
}

template <typename T>
std::array<T, 5>&& sort(std::array<T, 5>&& v)
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

        return std::move(v);
}
}
