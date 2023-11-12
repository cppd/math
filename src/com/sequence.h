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

#include <cstddef>
#include <type_traits>

namespace ns
{
namespace sequence_implementation
{
template <
        template <typename...>
        typename Type,
        template <std::size_t, typename...>
        typename SequenceType,
        typename... SequenceTypeParameters>
struct Sequence final
{
        template <int FIRST, int N, std::size_t... I>
        struct S final
        {
                static_assert(N > 0);
                using T = S<FIRST, N - 1, N - 1, I...>::T;
        };

        template <int FIRST, std::size_t... I>
        struct S<FIRST, 0, I...> final
        {
                static_assert(sizeof...(I) > 0);
                using T = Type<SequenceType<FIRST + I, SequenceTypeParameters...>...>;
        };
};

template <
        template <typename...>
        typename Type,
        template <std::size_t, typename...>
        typename SequenceType,
        typename... SequenceTypeParameters,
        template <typename, std::size_t...>
        typename IntegerSequence,
        typename IntegerType,
        std::size_t... I>
auto sequence(IntegerSequence<IntegerType, I...>&&)
{
        return std::add_pointer_t<Type<SequenceType<I, SequenceTypeParameters...>...>>();
}
}

// Type<SequenceType<From, ...>, SequenceType<From + 1, ...>, SequenceType<From + 2, ...>, ...>
template <
        template <typename...>
        typename Type,
        int FROM,
        int TO,
        template <std::size_t, typename...>
        typename SequenceType,
        typename... SequenceTypeParameters>
using SequenceRange = sequence_implementation::Sequence<Type, SequenceType, SequenceTypeParameters...>::
        template S<FROM, TO - FROM + 1>::T;

// Type<SequenceType<Index[0], ...>, SequenceType<index[1], ...>, SequenceType<index[2], ...>, ...>
template <
        typename IntegerSequence,
        template <typename...>
        typename Type,
        template <std::size_t, typename...>
        typename SequenceType,
        typename... SequenceTypeParameters>
using Sequence = std::remove_pointer_t<
        decltype(sequence_implementation::sequence<Type, SequenceType, SequenceTypeParameters...>(IntegerSequence()))>;
}
