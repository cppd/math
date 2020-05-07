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
template <
        template <typename...>
        typename Type,
        template <size_t, typename...>
        typename SequenceType,
        typename... SequenceTypeParameters>
struct Sequence
{
        template <int first, int N, size_t... I>
        struct S
        {
                static_assert(N > 0);
                using T = typename S<first, N - 1, N - 1, I...>::T;
        };
        template <int first, size_t... I>
        struct S<first, 0, I...>
        {
                static_assert(sizeof...(I) > 0);
                using T = Type<SequenceType<first + I, SequenceTypeParameters...>...>;
        };
};

template <
        template <typename...>
        typename Type,
        template <size_t, typename...>
        typename SequenceType,
        typename... SequenceTypeParameters,
        template <typename, size_t...>
        typename IntegerSequence,
        typename IntegerType,
        size_t... I>
auto sequence(IntegerSequence<IntegerType, I...>&&)
{
        return Type<SequenceType<I, SequenceTypeParameters...>...>();
}
}

// Тип Type<SequenceType<From, ...>, SequenceType<From + 1, ...>, SequenceType<From + 2, ...>, ...>
template <
        template <typename...>
        typename Type,
        int From,
        int To,
        template <size_t, typename...>
        typename SequenceType,
        typename... SequenceTypeParameters>
using SequenceRange = typename sequence_implementation::Sequence<Type, SequenceType, SequenceTypeParameters...>::
        template S<From, To - From + 1>::T;

// Тип Type<SequenceType<Index0, ...>, SequenceType<index1, ...>, SequenceType<index2, ...>, ...>
// где i берутся из IndexSequence (std::integer_sequence)
template <
        typename IntegerSequence,
        template <typename...>
        typename Type,
        template <size_t, typename...>
        typename SequenceType,
        typename... SequenceTypeParameters>
using Sequence =
        decltype(sequence_implementation::sequence<Type, SequenceType, SequenceTypeParameters...>(IntegerSequence()));
