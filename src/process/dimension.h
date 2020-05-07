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

#include <src/settings/dimensions.h>

namespace process
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
void apply_for_dimension(std::size_t dimension, const T& f)
{
        bool found = [&]<std::size_t... N>(std::index_sequence<N...> &&)
        {
                return ((N == dimension ? (f(Dimension<N>()), true) : false) || ...);
        }
        (settings::Dimensions());

        if (found)
        {
                return;
        }

        implementation::dimension_not_supported_error(dimension);
}
}
