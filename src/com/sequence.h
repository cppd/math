/*
Copyright (C) 2017-2020 Topological Manifold

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

#include <cstddef>

namespace sequence_implementation
{
template <template <typename...> typename SequenceType, template <size_t, typename...> typename Type, typename... Ts>
struct Sequence
{
        template <int first, int N, size_t... I>
        struct S
        {
                static_assert(N > 0);
                using V = typename S<first, N - 1, N - 1, I...>::V;
        };
        template <int first, size_t... I>
        struct S<first, 0, I...>
        {
                static_assert(sizeof...(I) > 0);
                using V = SequenceType<Type<first + I, Ts...>...>;
        };
};
}

// Тип SequenceType<T<from, ...>, T<From + 1, ...>, T<From + 2, ...>, ...>
template <
        template <typename...>
        typename SequenceType,
        int From,
        int To,
        template <size_t, typename...>
        typename T,
        typename... Ts>
using Sequence = typename sequence_implementation::Sequence<SequenceType, T, Ts...>::template S<From, To - From + 1>::V;
