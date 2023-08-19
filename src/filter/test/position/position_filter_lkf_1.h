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

#pragma once

#include "position_filter.h"

#include <memory>
#include <optional>

namespace ns::filter::test
{
template <std::size_t N, typename T>
std::unique_ptr<PositionFilter<N, T>> create_position_filter_lkf_1(T theta, T process_variance);
}
