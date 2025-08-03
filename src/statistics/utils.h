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

#include <src/numerical/vector.h>

#include <cmath>
#include <cstddef>
#include <type_traits>

namespace ns::statistics::utils
{
template <typename T>
        requires (std::is_floating_point_v<T>)
[[nodiscard]] T sqrt(const T a)
{
        return std::sqrt(a);
}

template <std::size_t N, typename T>
[[nodiscard]] numerical::Vector<N, T> sqrt(const numerical::Vector<N, T>& a)
{
        numerical::Vector<N, T> res;
        for (std::size_t i = 0; i < N; ++i)
        {
                res[i] = std::sqrt(a[i]);
        }
        return res;
}

template <typename T>
struct TypeTraits final
{
        using DataType = T;
};

template <std::size_t N, typename T>
struct TypeTraits<numerical::Vector<N, T>> final
{
        using DataType = T;
};
}
