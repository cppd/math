/*
Copyright (C) 2017, 2018 Topological Manifold

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

#if !defined(STD_VARIANT_NOT_FOUND)
#include <variant>
#else
#include "com/simple_variant.h"
#endif

#if !defined(STD_VARIANT_NOT_FOUND)

template <typename... T>
using Variant = std::variant<T...>;

template <typename Visitor, typename Variant>
void visit(const Visitor& visitor, Variant&& variant)
{
        std::visit(visitor, std::forward<Variant>(variant));
}

#else

template <typename... T>
using Variant = SimpleVariant<T...>;

template <typename Visitor, typename SimpleVariant>
void visit(const Visitor& visitor, SimpleVariant&& simple_variant)
{
        simple_visit(visitor, std::forward<SimpleVariant>(simple_variant));
}

#endif

namespace SequenceVariantImplementation
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
using SequenceVariant = typename SequenceVariantImplementation::SequenceVariant<Type, T...>::template S<from, to - from + 1>::V;
