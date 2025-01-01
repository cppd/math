/*
Copyright (C) 2017-2025 Topological Manifold

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

#include "image.h"

#include <array>
#include <cstddef>

namespace ns::image
{
struct Slice final
{
        std::size_t dimension;
        std::size_t coordinate;

        Slice()
        {
        }

        Slice(const std::size_t dimension, const std::size_t index)
                : dimension(dimension),
                  coordinate(index)
        {
        }
};

template <std::size_t N, std::size_t S>
Image<N - S> slice(const Image<N>& image, const std::array<Slice, S>& slices);
}
