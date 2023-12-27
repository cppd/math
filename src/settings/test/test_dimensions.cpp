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

#include "../dimensions.h"

#include <cstddef>
#include <tuple>
#include <utility>

namespace ns::settings
{
namespace
{
template <std::size_t INDEX, typename... T>
constexpr bool compare_two_elements(const std::tuple<T...>& t)
{
        if constexpr (INDEX + 1 == sizeof...(T))
        {
                return true;
        }
        else
        {
                return std::get<INDEX>(t) < std::get<INDEX + 1>(t);
        }
}

template <unsigned MIN, typename... T, std::size_t... I>
constexpr bool compare(const std::tuple<T...>& t, const std::index_sequence<I...>&)
{
        static_assert(sizeof...(T) > 0 && sizeof...(T) == sizeof...(I));

        return ((std::get<I>(t) >= MIN) && ...) && (compare_two_elements<I>(t) && ...);
}

template <unsigned MIN, template <typename T, T...> typename Sequence, typename T, T... I>
constexpr bool check(const Sequence<T, I...>&)
{
        return compare<MIN>(std::make_tuple(I...), std::make_index_sequence<sizeof...(I)>());
}

static_assert(check<3>(Dimensions()));
static_assert(check<2>(Dimensions2A()));
}
}
