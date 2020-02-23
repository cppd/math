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

#include <variant>

namespace sequence_variant_implementation
{
template <template <size_t, typename...> typename Type, typename... Ts>
struct SequenceVariant1
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
                using V = std::variant<Type<first + I, Ts...>...>;
        };
};

template <template <typename> typename Type1, template <size_t, typename...> typename Type2, typename... Ts>
struct SequenceVariant2
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
                using V = std::variant<Type1<Type2<first + I, Ts...>>...>;
        };
};
}

// Тип variant<T<from, ...>, T<From + 1, ...>, T<From + 2, ...>, ...>
template <int From, int To, template <size_t, typename...> typename T, typename... Ts>
using SequenceVariant1 =
        typename sequence_variant_implementation::SequenceVariant1<T, Ts...>::template S<From, To - From + 1>::V;

// Тип variant<T1<T2<From, ...>>, T1<T2<From + 1, ...>>, T1<T2<From + 2, ...>>, ...>
template <int From, int To, template <typename> typename T1, template <size_t, typename...> typename T2, typename... Ts>
using SequenceVariant2 =
        typename sequence_variant_implementation::SequenceVariant2<T1, T2, Ts...>::template S<From, To - From + 1>::V;

//

template <typename... T>
struct Visitors : T...
{
        using T::operator()...;
};
template <typename... T>
Visitors(T...)->Visitors<T...>;
