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

#include <src/settings/dimensions.h>

#include <cstddef>
#include <optional>
#include <type_traits>
#include <utility>

namespace ns::process
{
namespace dimension_implementation
{
template <template <std::size_t> typename Dimension, typename F, std::size_t... N>
using ReturnType = std::common_type_t<decltype(std::declval<F>()(Dimension<N>()))...>;

[[noreturn]] void dimension_not_supported_error(unsigned dimension);

template <template <std::size_t> typename Dimension, typename F, std::size_t... N>
        requires (std::is_same_v<void, ReturnType<Dimension, F, N...>>)
void apply(const std::size_t dimension, const F& f, std::index_sequence<N...>&&)
{
        const bool r = ((
                [&]
                {
                        if (N == dimension)
                        {
                                f(Dimension<N>());
                                return true;
                        }
                        return false;
                }()
                || ...));

        if (r)
        {
                return;
        }

        dimension_not_supported_error(dimension);
}

template <template <std::size_t> typename Dimension, typename F, std::size_t... N>
        requires (!std::is_same_v<void, ReturnType<Dimension, F, N...>>)
auto apply(const std::size_t dimension, const F& f, std::index_sequence<N...>&&)
{
        std::optional<ReturnType<Dimension, F, N...>> r;

        static_cast<void>((
                [&]
                {
                        if (N == dimension)
                        {
                                r.emplace(f(Dimension<N>()));
                                return true;
                        }
                        return false;
                }()
                || ...));

        if (r)
        {
                return std::move(*r);
        }

        dimension_not_supported_error(dimension);
}
}

template <std::size_t N>
struct Dimension final
{
};

template <typename T>
auto apply_for_dimension(const std::size_t dimension, const T& f)
{
        namespace impl = dimension_implementation;

        return impl::apply<Dimension>(dimension, f, settings::Dimensions());
}
}
