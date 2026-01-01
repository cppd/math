/*
Copyright (C) 2017-2026 Topological Manifold

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

#include "info.h"

#include <src/com/error.h>

#include <array>
#include <cstddef>
#include <type_traits>

namespace ns::painter::pixels::samples::com
{
namespace merge_implementation
{
using Signed = std::make_signed_t<std::size_t>;

template <typename A, typename B, typename Less, typename Copy, typename Sum>
void merge_full_low(const A& a, const B& b, const Less less, const Copy copy, const Sum sum)
{
        static_assert(Info<A>::COUNT == Info<B>::COUNT);
        static constexpr std::size_t COUNT = Info<A>::COUNT;
        static_assert(COUNT % 2 == 0);
        static_assert(COUNT >= 2);

        std::size_t a_i = 0;
        std::size_t b_i = 0;

        for (std::size_t i = 0; i < COUNT / 2; ++i)
        {
                if (less(a_i, b_i))
                {
                        copy(i, a_i++, a);
                }
                else
                {
                        copy(i, b_i++, b);
                }
        }

        while (a_i < COUNT / 2)
        {
                sum(a_i++, a);
        }

        while (b_i < COUNT / 2)
        {
                sum(b_i++, b);
        }
}

template <typename A, typename B, typename Greater, typename Copy, typename Sum>
void merge_full_high(const A& a, const B& b, const Greater greater, const Copy copy, const Sum sum)
{
        static_assert(Info<A>::COUNT == Info<B>::COUNT);
        static constexpr std::size_t COUNT = Info<A>::COUNT;
        static_assert(COUNT % 2 == 0);
        static_assert(COUNT >= 2);

        std::size_t a_i = COUNT - 1;
        std::size_t b_i = COUNT - 1;

        for (std::size_t i = COUNT - 1; i >= COUNT / 2; --i)
        {
                if (greater(a_i, b_i))
                {
                        copy(i, a_i--, a);
                }
                else
                {
                        copy(i, b_i--, b);
                }
        }

        while (a_i >= COUNT / 2)
        {
                sum(a_i--, a);
        }

        while (b_i >= COUNT / 2)
        {
                sum(b_i--, b);
        }
}

template <typename A, typename B, typename Less, typename Copy>
[[nodiscard]] std::array<Signed, 2> merge_partial_low(
        const A& a,
        const B& b,
        const std::size_t a_size,
        const std::size_t b_size,
        const Less less,
        const Copy copy)
{
        static_assert(Info<A>::COUNT == Info<B>::COUNT);
        static constexpr std::size_t COUNT = Info<A>::COUNT;
        static_assert(COUNT % 2 == 0);
        static_assert(COUNT >= 2);

        std::size_t a_i = 0;
        std::size_t b_i = 0;
        std::size_t i = 0;

        while (i < COUNT / 2 && a_i < a_size && b_i < b_size)
        {
                if (less(a_i, b_i))
                {
                        copy(i++, a_i++, a);
                }
                else
                {
                        copy(i++, b_i++, b);
                }
        }

        while (i < COUNT / 2 && a_i < a_size)
        {
                copy(i++, a_i++, a);
        }

        while (i < COUNT / 2 && b_i < b_size)
        {
                copy(i++, b_i++, b);
        }

        return {static_cast<Signed>(a_i), static_cast<Signed>(b_i)};
}

template <typename A, typename B, typename Greater, typename Copy>
[[nodiscard]] std::array<Signed, 2> merge_partial_high(
        const A& a,
        const B& b,
        const std::size_t a_size,
        const std::size_t b_size,
        const Signed a_min,
        const Signed b_min,
        const Greater greater,
        const Copy copy)
{
        static_assert(Info<A>::COUNT == Info<B>::COUNT);
        static constexpr std::size_t COUNT = Info<A>::COUNT;
        static_assert(COUNT % 2 == 0);
        static_assert(COUNT >= 2);

        Signed a_i = a_size - 1;
        Signed b_i = b_size - 1;
        std::size_t i = COUNT - 1;

        while (i >= COUNT / 2 && a_i >= a_min && b_i >= b_min)
        {
                if (greater(a_i, b_i))
                {
                        copy(i--, a_i--, a);
                }
                else
                {
                        copy(i--, b_i--, b);
                }
        }

        while (i >= COUNT / 2 && a_i >= a_min)
        {
                copy(i--, a_i--, a);
        }

        while (i >= COUNT / 2 && b_i >= b_min)
        {
                copy(i--, b_i--, b);
        }

        return {a_i, b_i};
}
}

template <typename A, typename B, typename Less, typename Greater, typename Copy, typename Sum>
void merge_with_sum(const A& a, const B& b, const Less less, const Greater greater, const Copy copy, const Sum sum)
{
        static_assert(Info<A>::COUNT == Info<B>::COUNT);
        static constexpr std::size_t COUNT = Info<A>::COUNT;

        namespace impl = merge_implementation;

        const std::size_t a_size = a.count();
        const std::size_t b_size = b.count();

        if (a_size == COUNT && b_size == COUNT)
        {
                impl::merge_full_low(a, b, less, copy, sum);
                impl::merge_full_high(a, b, greater, copy, sum);
                return;
        }

        ASSERT(a_size > 0 && b_size > 0);
        ASSERT(a_size + b_size > COUNT);
        ASSERT(a_size + b_size < 2 * COUNT);

        const auto [a_low, b_low] = impl::merge_partial_low(a, b, a_size, b_size, less, copy);
        const auto [a_high, b_high] = impl::merge_partial_high(a, b, a_size, b_size, a_low, b_low, greater, copy);

        for (auto i = a_low; i <= a_high; ++i)
        {
                sum(i, a);
        }

        for (auto i = b_low; i <= b_high; ++i)
        {
                sum(i, b);
        }
}

template <typename A, typename B, typename Less, typename Copy>
void merge(const A& a, const B& b, const Less less, const Copy copy)
{
        static_assert(Info<A>::COUNT == Info<B>::COUNT);

        const std::size_t a_size = a.count();
        const std::size_t b_size = b.count();

        ASSERT(a_size > 0 && b_size > 0);
        ASSERT(a_size + b_size <= Info<A>::COUNT);

        std::size_t i = 0;
        std::size_t a_i = 0;
        std::size_t b_i = 0;

        while (a_i < a_size && b_i < b_size)
        {
                if (less(a_i, b_i))
                {
                        copy(i++, a_i++, a);
                }
                else
                {
                        copy(i++, b_i++, b);
                }
        }

        while (a_i < a_size)
        {
                copy(i++, a_i++, a);
        }

        while (b_i < b_size)
        {
                copy(i++, b_i++, b);
        }
}
}
