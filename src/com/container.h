/*
Copyright (C) 2017-2023 Topological Manifold

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

#include <iterator>
#include <type_traits>

namespace ns
{
namespace container_implementation
{
template <typename T>
concept Container =
        requires (const T& v) {
                std::data(v);
                std::size(v);
                v[0];
        };
}

template <typename T>
std::size_t data_size(const T& data)
{
        static_assert(!std::is_pointer_v<T>);
        if constexpr (container_implementation::Container<T>)
        {
                static_assert(std::is_trivially_copyable_v<typename T::value_type>);
                return std::size(data) * sizeof(typename T::value_type);
        }
        else
        {
                static_assert(std::is_trivially_copyable_v<T>);
                return sizeof(T);
        }
}

template <typename T>
auto data_pointer(T& data)
{
        static_assert(!std::is_pointer_v<T>);
        if constexpr (container_implementation::Container<T>)
        {
                static_assert(std::is_trivially_copyable_v<typename T::value_type>);
                return std::data(data);
        }
        else
        {
                static_assert(std::is_trivially_copyable_v<T>);
                return &data;
        }
}
}
