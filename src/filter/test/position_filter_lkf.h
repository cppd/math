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

#include <src/numerical/vector.h>

#include <memory>

namespace ns::filter::test
{
template <typename T>
std::unique_ptr<PositionFilter<T>> create_position_filter_lkf(
        const Vector<2, T>& position,
        T position_variance,
        T theta,
        T process_variance);
}
