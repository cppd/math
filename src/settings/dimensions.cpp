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

#include "dimensions.h"

#include <tuple>

namespace ns::settings
{
namespace
{
template <std::size_t Index, typename... T>
constexpr bool compare_two_elements(const std::tuple<T...>& t)
{
        if constexpr (Index + 1 == sizeof...(T))
        {
                return true;
        }
        else
        {
                return std::get<Index>(t) < std::get<Index + 1>(t);
        }
}

template <typename... T, std::size_t... I>
constexpr bool compare(std::tuple<T...>&& t, std::index_sequence<I...>&&)
{
        static_assert(sizeof...(T) > 0 && sizeof...(T) == sizeof...(I));

        return ((std::get<I>(t) >= 3) && ...) && (compare_two_elements<I>(t) && ...);
}

template <template <typename T, T...> typename Sequence, typename T, T... I>
constexpr bool check(const Sequence<T, I...>&)
{
        return compare(std::make_tuple(I...), std::make_index_sequence<sizeof...(I)>());
}

static_assert(check(Dimensions()));
}
}
