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
#include <type_traits>

namespace sequence_implementation
{
template <template <typename...> typename SequenceType, template <size_t, typename...> typename Type, typename... Ts>
struct Sequence1
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

template <
        template <typename...>
        typename SequenceType,
        bool ConstType2,
        template <typename>
        typename Type1,
        template <size_t, typename...>
        typename Type2,
        typename... Ts>
struct Sequence2
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
                using V = SequenceType<Type1<std::conditional_t<
                        ConstType2,
                        std::add_const_t<Type2<first + I, Ts...>>,
                        std::remove_const_t<Type2<first + I, Ts...>>>>...>;
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
using SequenceType1 =
        typename sequence_implementation::Sequence1<SequenceType, T, Ts...>::template S<From, To - From + 1>::V;

// Тип SequenceType<T1<T2<From, ...>>, T1<T2<From + 1, ...>>, T1<T2<From + 2, ...>>, ...>
// GCC 9 не работает для задания типа как const передачу шаблона template ... using ... = const T,
// поэтому используется параметр для const. Clang 9 работает.
template <
        template <typename...>
        typename SequenceType,
        int From,
        int To,
        template <typename>
        typename T1,
        template <size_t, typename...>
        typename T2,
        typename... Ts>
using SequenceType2ConstType2 = typename sequence_implementation::Sequence2<SequenceType, true, T1, T2, Ts...>::
        template S<From, To - From + 1>::V;

//
