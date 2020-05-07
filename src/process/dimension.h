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

#include <src/com/sequence.h>
#include <src/settings/dimensions.h>

#include <tuple>

namespace process
{
template <size_t N>
struct Dimension
{
};

inline constexpr auto DIMENSIONS = Sequence<settings::Dimensions, std::tuple, Dimension>();

//

[[noreturn]] void dimension_not_supported_error(unsigned dimension);

//

template <typename T>
void apply_for_dimension(size_t dimension, const T& f)
{
        bool found = std::apply(
                [&]<size_t... N>(const Dimension<N>&...) {
                        return ([&]() {
                                if (N == dimension)
                                {
                                        f(Dimension<N>());
                                        return true;
                                }
                                return false;
                        }() || ...);
                },
                DIMENSIONS);

        if (found)
        {
                return;
        }

        dimension_not_supported_error(dimension);
}
}
