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

#include "view/point.h"

#include <src/filter/filters/filter.h>

#include <cstddef>
#include <vector>

namespace ns::filter::test
{
template <std::size_t N, typename T, std::size_t ORDER>
[[nodiscard]] std::vector<view::Point<2, T>> smooth(
        const filters::FilterPosition<2, T, ORDER>& filter,
        const std::vector<filters::UpdateDetails<N, T>>& details);

template <std::size_t N, typename T, std::size_t ORDER>
[[nodiscard]] std::vector<view::Point<2, T>> smooth(
        const filters::FilterPosition<2, T, ORDER>& filter,
        const std::vector<filters::UpdateDetails<N, T>>& details,
        unsigned lag);
}
