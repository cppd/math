/*
Copyright (C) 2017-2022 Topological Manifold

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

#include <optional>

namespace ns::process
{
namespace implementation
{
[[noreturn]] void dimension_not_supported_error(unsigned dimension);
}

template <std::size_t N>
struct Dimension
{
};

template <typename T>
auto apply_for_dimension(const std::size_t dimension, const T& f)
{
        return [&]<std::size_t... N>(std::index_sequence<N...> &&)
        {
                using ReturnType = std::common_type_t<decltype(f(Dimension<N>()))...>;

                if constexpr (std::is_same_v<void, ReturnType>)
                {
                        if (((N == dimension ? (static_cast<void>(f(Dimension<N>())), true) : false) || ...))
                        {
                                return;
                        }
                }
                else
                {
                        std::optional<ReturnType> r;
                        if (((N == dimension ? (static_cast<void>(r.emplace(f(Dimension<N>()))), true) : false) || ...))
                        {
                                return std::move(*r);
                        }
                }

                implementation::dimension_not_supported_error(dimension);
        }
        (settings::Dimensions());
}
}
