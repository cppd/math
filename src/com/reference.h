/*
Copyright (C) 2017-2024 Topological Manifold

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

#include <utility>

namespace ns
{
namespace reference_implementation
{
template <typename T>
concept Indirection = requires (T&& v) { *v; };
}

template <typename T>
[[nodiscard]] decltype(auto) to_ref(T* const object)
{
        return *object;
}

template <typename T>
        requires (!reference_implementation::Indirection<T>)
[[nodiscard]] decltype(auto) to_ref(T&& object)
{
        return std::forward<T>(object);
}

template <typename T>
        requires (reference_implementation::Indirection<T>)
[[nodiscard]] decltype(auto) to_ref(T& object)
{
        return *object;
}

template <typename T>
        requires (reference_implementation::Indirection<T>)
void to_ref(const T&& object) = delete;
}
