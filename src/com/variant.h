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

#if 1

#include <variant>

template <typename... T>
using Variant = std::variant<T...>;

template <typename Visitor, typename Variant>
void visit(const Visitor& visitor, Variant&& variant)
{
        std::visit(visitor, std::forward<Variant>(variant));
}

template <typename T, typename... Types>
const T& get(const std::variant<Types...>& variant)
{
        return std::get<T>(variant);
}

#else

#include "com/simple_variant.h"

template <typename... T>
using Variant = SimpleVariant<T...>;

template <typename Visitor, typename SimpleVariant>
void visit(const Visitor& visitor, SimpleVariant&& simple_variant)
{
        simple_visit(visitor, std::forward<SimpleVariant>(simple_variant));
}

template <typename T, typename... Types>
const T& get(const SimpleVariant<Types...>& variant)
{
        return simple_get<T>(variant);
}

#endif

namespace sequence_variant_implementation
{
template <template <size_t, typename...> typename Type, typename... T>
struct SequenceVariant
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
                using V = Variant<Type<first + I, T...>...>;
        };
};
}

// Тип variant<Type<from>, Type<From + 1>, Type<From + 2>, ...>
template <int from, int to, template <size_t, typename...> typename Type, typename... T>
using SequenceVariant = typename sequence_variant_implementation::SequenceVariant<Type, T...>::template S<from, to - from + 1>::V;

//

template <typename... T>
struct Visitors : T...
{
        using T::operator()...;
};
template <typename... T>
Visitors(T...)->Visitors<T...>;
