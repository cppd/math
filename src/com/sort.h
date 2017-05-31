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
#ifndef SORT_H
#define SORT_H

#include <algorithm>
#include <array>

template <typename T, size_t N>
std::array<T, N>&& sort(std::array<T, N>&& v)
{
        std::sort(v.begin(), v.end());
        return std::move(v);
}

//Эти функции сортировки работают немного быстрее, чем std:sort

template <typename T>
void swap(T& a, T& b)
{
        if (b < a)
        {
                T tmp = std::move(a);
                a = std::move(b);
                b = std::move(tmp);
        }
}
template <typename T>
std::array<T, 2>&& sort(std::array<T, 2>&& v)
{
        swap(v[0], v[1]);

        return std::move(v);
}
template <typename T>
std::array<T, 3>&& sort(std::array<T, 3>&& v)
{
        swap(v[0], v[1]);
        swap(v[0], v[2]);
        swap(v[1], v[2]);

        return std::move(v);
}
template <typename T>
std::array<T, 4>&& sort(std::array<T, 4>&& v)
{
        swap(v[0], v[1]);
        swap(v[2], v[3]);
        swap(v[0], v[2]);
        swap(v[1], v[3]);
        swap(v[1], v[2]);

        return std::move(v);
}
template <typename T>
std::array<T, 5>&& sort(std::array<T, 5>&& v)
{
        swap(v[0], v[1]);
        swap(v[2], v[3]);
        swap(v[1], v[3]);
        swap(v[3], v[4]);

        swap(v[0], v[1]);
        swap(v[2], v[3]);
        swap(v[0], v[2]);
        swap(v[1], v[3]);
        swap(v[1], v[2]);

        return std::move(v);
}

#endif
